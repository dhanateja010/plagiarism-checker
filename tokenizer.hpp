#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>

// Lightweight tokenizer used by the plagiarism checker.
class tokenizer_t {
private:
    std::string file_name;

public:
    explicit tokenizer_t(std::string __file_name);
    ~tokenizer_t() = default;

    std::vector<int> get_tokens();
};

// Helper function for compatibility with the existing interface.
std::string get_cursor_kind_spelling(int kind);

#endif // TOKENIZER_HPP