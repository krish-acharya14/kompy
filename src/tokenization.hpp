#pragma once

#include <cctype>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    exit, assume, maybe, otherwise,

    // Identifiers and literals
    identifier, integer,

    // Symbols
    open_paren, close_paren,   // ( )
    open_brace, close_brace,   // { }
    semicolon,

    // Assignment
    equal,

    // Arithmetic
    plus, minus, star, slash, percent,

    // Comparison
    equal_equal, not_equal, less, less_equal, greater, greater_equal,

    // Logic
    and_and, or_or, not_op
};

struct Token {
    TokenType type;
    std::optional<std::string> value;
};

class Tokenizer {
    public:
        inline explicit Tokenizer(std::string src) : m_src(std::move(src)), m_index(0) {}

        inline std::vector<Token> tokenize() {
            std::vector<Token> tokens;
            std::string buf;
            while (peek().has_value()) {
                if (std::isalpha(peek().value())) {
                    buf.push_back(consume());
                    while (peek().has_value() && std::isalnum(peek().value())) 
                        buf.push_back(consume());
                    
                    if (buf == "getback") tokens.push_back({ .type = TokenType::exit });
                    else if (buf == "assume") tokens.push_back({ .type = TokenType::assume });
                    else if (buf == "maybe") tokens.push_back({ .type = TokenType::maybe });
                    else if (buf == "otherwise") tokens.push_back({ .type = TokenType::otherwise });
                    else tokens.push_back({ .type = TokenType::identifier, .value = buf });
                    buf.clear();
                    continue;
                }

                else if (std::isdigit(peek().value())) {
                    buf.push_back(consume());
                    while (peek().has_value() && std::isdigit(peek().value()))
                        buf.push_back(consume());
                    tokens.push_back({ .type = TokenType::integer, .value = buf });
                    buf.clear();
                    continue;
                }

                else if (std::isspace(peek().value())) {
                    consume();
                    continue;
                }

                else if (peek().has_value()) {
                    switch (peek().value()) {
                        case '(': tokens.push_back({ .type = TokenType::open_paren }); consume(); break;
                        case ')': tokens.push_back({ .type = TokenType::close_paren }); consume(); break;
                        case '{': tokens.push_back({ .type = TokenType::open_brace }); consume(); break;
                        case '}': tokens.push_back({ .type = TokenType::close_brace }); consume(); break;
                        case ';': tokens.push_back({ .type = TokenType::semicolon }); consume(); break;
                        case '=': 
                            consume();
                            if (peek().has_value() && peek().value() == '=') {
                                tokens.push_back({ .type = TokenType::equal_equal });
                                consume();
                            } else {
                                tokens.push_back({ .type = TokenType::equal });
                            }
                            break;
                        case '+': tokens.push_back({ .type = TokenType::plus }); consume(); break;
                        case '-': tokens.push_back({ .type = TokenType::minus }); consume(); break;
                        case '*': tokens.push_back({ .type = TokenType::star }); consume(); break;
                        case '/': tokens.push_back({ .type = TokenType::slash }); consume(); break;
                        case '%': tokens.push_back({ .type = TokenType::percent }); consume(); break;
                        case '!':
                            consume();
                            if (peek().has_value() && peek().value() == '=') {
                                tokens.push_back({ .type = TokenType::not_equal });
                                consume();
                            } else {
                                std::cerr << "Unexpected token: !" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            break;
                        case '<':
                            consume();
                            if (peek().has_value() && peek().value() == '=') {
                                tokens.push_back({ .type = TokenType::less_equal });
                                consume();
                            } else {
                                tokens.push_back({ .type = TokenType::less });
                            }
                            break;
                        case '>':
                            consume();
                            if (peek().has_value() && peek().value() == '=') {
                                tokens.push_back({ .type = TokenType::greater_equal });
                                consume();
                            } else {
                                tokens.push_back({ .type = TokenType::greater });
                            }
                            break;
                        case '&':
                            consume();
                            if (peek().has_value() && peek().value() == '&') {
                                tokens.push_back({ .type = TokenType::and_and });
                                consume();
                            } else {
                                std::cerr << "Unexpected token: &" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            break;
                        case '|':
                            consume();
                            if (peek().has_value() && peek().value() == '|') {
                                tokens.push_back({ .type = TokenType::or_or });
                                consume();
                            } else {
                                std::cerr << "Unexpected token: |" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            break;
                    }
                }

                else {
                    std::cerr << "Unexpected character: " << peek().value() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            m_index = 0;
            return tokens;
        } 

    private:
        [[nodiscard]] inline std::optional<char> peek(size_t offset = 0) const {
            if (m_index + offset >= m_src.size())
                return {};
            return m_src[m_index + offset];
        }

        inline char consume() { return m_src.at(m_index++); }

        std::string m_src;
        size_t m_index;

};
