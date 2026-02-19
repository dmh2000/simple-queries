#include "super.h"

#include <algorithm>
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

// --- Argument parsing ---

Args parse_args(int argc, const char* argv[]) {
    Args args;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--base-url" && i + 1 < argc) {
            args.base_url = argv[++i];
        } else if (arg == "--model" && i + 1 < argc) {
            args.model = argv[++i];
        } else if (arg == "--api-key" && i + 1 < argc) {
            args.api_key = argv[++i];
        }
    }

    if (args.base_url.empty() || args.model.empty() || args.api_key.empty()) {
        throw std::runtime_error("Usage: super --base-url URL --model MODEL --api-key ENV_VAR");
    }

    return args;
}

// --- JSON helpers ---

static std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}

std::string build_request_json(const std::string& model, const std::string& content) {
    return "{\"model\":\"" + escape_json(model) + "\","
           "\"messages\":[{\"role\":\"user\",\"content\":\"" + escape_json(content) + "\"}]}";
}

// Minimal JSON string extractor: find the value of a key at a given search position.
// Returns the unescaped string value, or throws on failure.
static std::string extract_json_string(const std::string& json, const std::string& key,
                                       std::string::size_type start = 0) {
    std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle, start);
    if (pos == std::string::npos) {
        throw std::runtime_error("key '" + key + "' not found in JSON");
    }

    // Skip past key and colon
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) {
        throw std::runtime_error("malformed JSON after key '" + key + "'");
    }
    pos++;

    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.size() || json[pos] != '"') {
        throw std::runtime_error("expected string value for key '" + key + "'");
    }
    pos++; // skip opening quote

    std::string value;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            pos++;
            switch (json[pos]) {
                case '"':  value += '"'; break;
                case '\\': value += '\\'; break;
                case 'n':  value += '\n'; break;
                case 'r':  value += '\r'; break;
                case 't':  value += '\t'; break;
                default:   value += json[pos];
            }
        } else {
            value += json[pos];
        }
        pos++;
    }

    return value;
}

std::string parse_response_json(const std::string& json) {
    // Check for "choices":[]
    auto choices_pos = json.find("\"choices\"");
    if (choices_pos == std::string::npos) {
        throw std::runtime_error("no choices in response");
    }

    // Find the opening bracket
    auto bracket = json.find('[', choices_pos);
    if (bracket == std::string::npos) {
        throw std::runtime_error("malformed response: no choices array");
    }

    // Check if the array is empty
    auto after_bracket = bracket + 1;
    while (after_bracket < json.size() && (json[after_bracket] == ' ' || json[after_bracket] == '\t' || json[after_bracket] == '\n')) {
        after_bracket++;
    }
    if (after_bracket < json.size() && json[after_bracket] == ']') {
        throw std::runtime_error("no choices in response");
    }

    // Extract content from the first choice's message
    return extract_json_string(json, "content", choices_pos);
}

// --- HTTP via POSIX sockets ---

struct ParsedURL {
    std::string host;
    std::string port;
    std::string path;
};

static ParsedURL parse_url(const std::string& url) {
    ParsedURL parsed;

    // Skip scheme
    auto pos = url.find("://");
    if (pos == std::string::npos) {
        throw std::runtime_error("invalid URL: " + url);
    }
    pos += 3;

    // Extract host:port
    auto path_start = url.find('/', pos);
    std::string host_port;
    if (path_start == std::string::npos) {
        host_port = url.substr(pos);
        parsed.path = "/";
    } else {
        host_port = url.substr(pos, path_start - pos);
        parsed.path = url.substr(path_start);
    }

    auto colon = host_port.find(':');
    if (colon == std::string::npos) {
        parsed.host = host_port;
        parsed.port = "80";
    } else {
        parsed.host = host_port.substr(0, colon);
        parsed.port = host_port.substr(colon + 1);
    }

    return parsed;
}

std::string execute_query(const std::string& base_url, const std::string& model,
                          const std::string& api_key, const std::string& content) {
    ParsedURL url = parse_url(base_url);
    std::string path = url.path + "/chat/completions";

    std::string body = build_request_json(model, content);

    // Resolve host
    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(url.host.c_str(), url.port.c_str(), &hints, &res);
    if (err != 0) {
        throw std::runtime_error(std::string("DNS resolution failed: ") + gai_strerror(err));
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        throw std::runtime_error("failed to create socket");
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(fd);
        freeaddrinfo(res);
        throw std::runtime_error("connection failed");
    }
    freeaddrinfo(res);

    // Build HTTP request
    std::ostringstream req;
    req << "POST " << path << " HTTP/1.1\r\n"
        << "Host: " << url.host << ":" << url.port << "\r\n"
        << "Authorization: Bearer " << api_key << "\r\n"
        << "Content-Type: application/json\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;

    std::string request = req.str();
    ssize_t sent = write(fd, request.c_str(), request.size());
    if (sent < 0) {
        close(fd);
        throw std::runtime_error("failed to send request");
    }

    // Read response
    std::string response;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        response.append(buf, n);
    }
    close(fd);

    // Parse HTTP status
    auto status_start = response.find(' ');
    if (status_start == std::string::npos) {
        throw std::runtime_error("malformed HTTP response");
    }
    int status_code = std::stoi(response.substr(status_start + 1, 3));

    // Find body (after \r\n\r\n)
    auto body_start = response.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        throw std::runtime_error("malformed HTTP response: no body");
    }
    std::string resp_body = response.substr(body_start + 4);

    if (status_code != 200) {
        throw std::runtime_error("API returned status " + std::to_string(status_code) + ": " + resp_body);
    }

    return parse_response_json(resp_body);
}
