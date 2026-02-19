#include "super.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void name(); \
    static struct name##_reg { \
        name##_reg() { run(#name, name); } \
    } name##_instance; \
    static void run_##name()

#define RUN_TEST(name) \
    do { \
        tests_run++; \
        std::cout << "  " << #name << "... "; \
        try { \
            name(); \
            tests_passed++; \
            std::cout << "PASS" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << "FAIL: " << e.what() << std::endl; \
        } \
    } while(0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            throw std::runtime_error( \
                std::string("expected '") + std::string(b) + "' got '" + std::string(a) + "'"); \
        } \
    } while(0)

#define ASSERT_THROWS(expr) \
    do { \
        bool threw = false; \
        try { expr; } catch (...) { threw = true; } \
        if (!threw) throw std::runtime_error("expected exception but none thrown"); \
    } while(0)

#define ASSERT_THROWS_CONTAINING(expr, substr) \
    do { \
        bool threw = false; \
        try { expr; } catch (const std::exception& e) { \
            threw = true; \
            if (std::string(e.what()).find(substr) == std::string::npos) \
                throw std::runtime_error( \
                    std::string("exception '") + e.what() + "' doesn't contain '" + substr + "'"); \
        } \
        if (!threw) throw std::runtime_error("expected exception but none thrown"); \
    } while(0)

// --- Fake HTTP server using POSIX sockets ---

struct FakeServer {
    int port;
    pid_t pid;
};

// Start a fake server that accepts one connection and sends the given response.
static FakeServer start_fake_server(int status_code, const std::string& body) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(server_fd >= 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // let OS pick

    assert(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    assert(listen(server_fd, 1) == 0);

    socklen_t len = sizeof(addr);
    getsockname(server_fd, (struct sockaddr*)&addr, &len);
    int port = ntohs(addr.sin_port);

    pid_t pid = fork();
    if (pid == 0) {
        // Child: accept one connection, read request, send response, exit
        int client_fd = accept(server_fd, nullptr, nullptr);
        close(server_fd);

        // Read the full request (we just drain it)
        char buf[4096];
        read(client_fd, buf, sizeof(buf));

        std::string status_text = (status_code == 200) ? "OK" : "Error";
        std::string response =
            "HTTP/1.1 " + std::to_string(status_code) + " " + status_text + "\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" + body;

        write(client_fd, response.c_str(), response.size());
        close(client_fd);
        _exit(0);
    }

    close(server_fd);
    return {port, pid};
}

static void stop_fake_server(FakeServer& s) {
    int status;
    waitpid(s.pid, &status, 0);
}

// --- Tests ---

void test_parse_args_all_flags() {
    const char* argv[] = {"super", "--base-url", "http://localhost", "--model", "gpt-4", "--api-key", "MY_KEY"};
    Args args = parse_args(7, argv);
    ASSERT_EQ(args.base_url, "http://localhost");
    ASSERT_EQ(args.model, "gpt-4");
    ASSERT_EQ(args.api_key, "MY_KEY");
}

void test_parse_args_missing_required() {
    const char* argv[] = {"super", "--base-url", "http://localhost"};
    ASSERT_THROWS(parse_args(3, argv));
}

void test_build_request_json() {
    std::string json = build_request_json("test-model", "hello");
    // Verify it contains expected fields
    assert(json.find("\"model\":\"test-model\"") != std::string::npos
        || json.find("\"model\": \"test-model\"") != std::string::npos);
    assert(json.find("\"role\":\"user\"") != std::string::npos
        || json.find("\"role\": \"user\"") != std::string::npos);
    assert(json.find("\"content\":\"hello\"") != std::string::npos
        || json.find("\"content\": \"hello\"") != std::string::npos);
}

void test_parse_response_json_success() {
    std::string json = R"({"choices":[{"message":{"role":"assistant","content":"Hi there!"}}]})";
    std::string content = parse_response_json(json);
    ASSERT_EQ(content, "Hi there!");
}

void test_parse_response_json_empty_choices() {
    std::string json = R"({"choices":[]})";
    ASSERT_THROWS_CONTAINING(parse_response_json(json), "no choices");
}

void test_parse_response_json_invalid() {
    ASSERT_THROWS(parse_response_json("not json"));
}

void test_execute_query_success() {
    std::string body = R"({"choices":[{"message":{"role":"assistant","content":"Hi there!"}}]})";
    FakeServer s = start_fake_server(200, body);

    std::string base_url = "http://127.0.0.1:" + std::to_string(s.port);
    std::string result = execute_query(base_url, "test-model", "test-key", "hello");
    ASSERT_EQ(result, "Hi there!");

    stop_fake_server(s);
}

void test_execute_query_api_error() {
    std::string body = R"({"error":"invalid api key"})";
    FakeServer s = start_fake_server(401, body);

    std::string base_url = "http://127.0.0.1:" + std::to_string(s.port);
    ASSERT_THROWS_CONTAINING(
        execute_query(base_url, "test-model", "bad-key", "hello"),
        "401"
    );

    stop_fake_server(s);
}

void test_execute_query_empty_choices() {
    std::string body = R"({"choices":[]})";
    FakeServer s = start_fake_server(200, body);

    std::string base_url = "http://127.0.0.1:" + std::to_string(s.port);
    ASSERT_THROWS_CONTAINING(
        execute_query(base_url, "test-model", "test-key", "hello"),
        "no choices"
    );

    stop_fake_server(s);
}

int main() {
    std::cout << "Running tests..." << std::endl;

    RUN_TEST(test_parse_args_all_flags);
    RUN_TEST(test_parse_args_missing_required);
    RUN_TEST(test_build_request_json);
    RUN_TEST(test_parse_response_json_success);
    RUN_TEST(test_parse_response_json_empty_choices);
    RUN_TEST(test_parse_response_json_invalid);
    RUN_TEST(test_execute_query_success);
    RUN_TEST(test_execute_query_api_error);
    RUN_TEST(test_execute_query_empty_choices);

    std::cout << std::endl << tests_passed << "/" << tests_run << " tests passed." << std::endl;
    return (tests_passed == tests_run) ? 0 : 1;
}
