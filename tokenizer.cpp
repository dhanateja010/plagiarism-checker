#include "tokenizer.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

tokenizer_t::tokenizer_t(std::string __file_name) : file_name(std::move(__file_name)) {}

std::vector<int> tokenizer_t::get_tokens() {
    std::ifstream input(file_name);
    if (!input.is_open()) {
        throw std::invalid_argument("Unable to parse file: " + file_name);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    std::string content = buffer.str();
    std::vector<int> tokens;
    tokens.reserve(content.size());

    for (char ch : content) {
        unsigned char value = static_cast<unsigned char>(ch);
        if (std::isalnum(value)) {
            tokens.push_back(static_cast<int>(std::tolower(value)));
        } else if (std::isspace(value)) {
            tokens.push_back(static_cast<int>(' '));
        } else {
            tokens.push_back(static_cast<int>(value));
        }
    }

    return tokens;
}

std::string get_cursor_kind_spelling(int kind) {
    return "token-" + std::to_string(kind);
}