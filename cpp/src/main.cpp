#include "super.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, const char* argv[]) {
    Args args;
    try {
        args = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    const char* api_key_val = std::getenv(args.api_key.c_str());
    if (!api_key_val || api_key_val[0] == '\0') {
        std::cerr << "Error: " << args.api_key << " environment variable not set" << std::endl;
        return 1;
    }
    std::string api_key = api_key_val;

    std::ostringstream ss;
    ss << std::cin.rdbuf();
    std::string content = ss.str();

    // Trim whitespace
    auto start = content.find_first_not_of(" \t\n\r");
    auto end = content.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) {
        std::cerr << "Error: No input provided on stdin" << std::endl;
        return 1;
    }
    content = content.substr(start, end - start + 1);

    std::cerr << "base_url: " << args.base_url << std::endl;
    std::cerr << "model:    " << args.model << std::endl;
    std::cerr << "api_key:  ******** (from " << args.api_key << ")" << std::endl;
    std::cerr << "prompt:   " << content << std::endl;

    try {
        std::string result = execute_query(args.base_url, args.model, api_key, content);
        std::cout << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
