#pragma once

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    exit, assume,

    //Identifiers and literals
    identifier, integer,

    // Symbols
    open_paren, close_paren, semicolon,

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
                    while (peek().has_value() && (std::isalnum(peek().value()))) 
                        buf.push_back(consume());
                    
                    if (buf == "getback") tokens.push_back({ .type = TokenType::exit });
                    else if (buf == "assume") tokens.push_back({ .type = TokenType::assume });
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
                else if (peek().value() == '(') {
                    consume();
                    tokens.push_back({ .type = TokenType::open_paren });
                    continue;
                }
                else if (peek().value() == ')'){
                    consume();
                    tokens.push_back({ .type = TokenType::close_paren });
                    continue;
                }
                else if (peek().value() == ';') {
                    consume();
                    tokens.push_back({ .type = TokenType::semicolon });
                    continue;
                }

                else if (peek().value() == '+') {
                    consume();
                    tokens.push_back({ .type = TokenType::plus });
                    continue;
                }
                else if (peek().value() == '-') {
                    consume();
                    tokens.push_back({ .type = TokenType::minus});
                    continue;
                }
                else if (peek().value() == '*') {
                    consume();
                    tokens.push_back({ .type = TokenType::star});
                    continue;
                }
                else if (peek().value() == '/') {
                    consume();
                    tokens.push_back({ .type = TokenType::slash});
                    continue;
                }
                else if (peek().value() == '%') {
                    consume();
                    tokens.push_back({ .type = TokenType::percent});
                    continue;
                }

                else if (peek().value() == '=') {
                    consume();
                    if (peek().has_value() && peek().value() == '=') {
                        consume();
                        tokens.push_back({ .type = TokenType::equal_equal });
                    }
                    else tokens.push_back({ .type = TokenType::equal });
                    continue;
                }
                else if (peek().value() == '!') {
                    consume();
                    if (peek().has_value() && peek().value() == '=') {
                        consume();
                        tokens.push_back({ .type = TokenType::not_equal });
                    }
                    else tokens.push_back({ .type = TokenType::not_op });
                    continue;
                }
                else if (peek().value() == '<') {
                    consume();
                    if (peek().has_value() && peek().value() == '=') {
                        consume();
                        tokens.push_back({ .type = TokenType::less_equal });
                    }
                    else tokens.push_back({ .type = TokenType::less });
                    continue;
                }
                else if (peek().value() == '>') {
                    consume();
                    if (peek().has_value() && peek().value() == '=') {
                        consume();
                        tokens.push_back({ .type = TokenType::greater_equal });
                    }
                    else tokens.push_back({ .type = TokenType::greater });
                    continue;
                }

                else if (peek().value() == '&') {
                    consume();
                    if (peek().has_value() && peek().value() == '&') {
                        consume();
                        tokens.push_back({ .type = TokenType::and_and });
                    }
                    else {
                        std::cerr << "Unexpected character &" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }
                else if (peek().value() == '|') {
                    consume();
                    if (peek().has_value() && peek().value() == '|') {
                        consume();
                        tokens.push_back({ .type = TokenType::or_or });
                    }
                    else {
                        std::cerr << "Unexpected character |" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }
                
                else if (std::isspace(peek().value())) {
                    consume();
                    continue;
                }
                else {
                    std::cerr << "Unexpected character " << peek().value() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            m_index = 0;
            return tokens;
        }
    private:
        [[nodiscard]] inline std::optional<char> peek(int offset = 0) const {
            if (m_index + offset >= m_src.size()) return {};
            return m_src.at(m_index + offset);
        }
        inline char consume() { return m_src.at(m_index++); }

        std::string m_src;
        std::size_t m_index;
};
