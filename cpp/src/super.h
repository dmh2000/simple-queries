#ifndef SUPER_H
#define SUPER_H

#include <string>
#include <stdexcept>

struct Args {
    std::string base_url;
    std::string model;
    std::string api_key;
};

// Parse command-line arguments. Throws std::runtime_error on missing required flags.
Args parse_args(int argc, const char* argv[]);

// Build the JSON request body for the chat completions API.
std::string build_request_json(const std::string& model, const std::string& content);

// Extract the content string from a chat completions JSON response.
// Throws std::runtime_error if the response is malformed or has no choices.
std::string parse_response_json(const std::string& json);

// Send a prompt to the chat completions API and return the response content.
// Throws std::runtime_error on network or API errors.
std::string execute_query(const std::string& base_url, const std::string& model,
                          const std::string& api_key, const std::string& content);

#endif
