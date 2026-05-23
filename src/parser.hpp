#pragma once

#include <memory>
#include <variant>
#include "tokenization.hpp"

struct NodeExpr;

enum class BinOp {
    Add, Sub, Mul, Div, Mod,      // Arithmetic
    Eq, Neq, Lt, Gt, Leq, Geq,    // Comparison
    And, Or                       // Logic
};

enum class UnaryOp {
    Not, Neg
};

struct NodeInteger {
    Token integer;
};

struct NodeIdentifier {
    Token identifier;
};

struct NodeBinExpr {
    BinOp op;
    std::shared_ptr<NodeExpr> left;
    std::shared_ptr<NodeExpr> right;
};

struct NodeUnaryExpr {
    UnaryOp op;
    std::shared_ptr<NodeExpr> operand;
};

struct NodeExpr {
    std::variant<NodeInteger, NodeIdentifier, NodeBinExpr, NodeUnaryExpr> var;
};

struct NodeExit {
    NodeExpr expr;
};

struct NodeAssume {
    Token identifier;
    NodeExpr expr;
};

struct NodeStmt {
    std::variant<NodeExit, NodeAssume> var;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)), m_index(0) {}

        static int bin_prec(TokenType t) {
            switch (t) {

                // Precedence order: 1 < 2 < 3 < 4 < 5 < 6
                // or -> 1 ; and -> 2 ; ==, != -> 3 ; <, <=, >, >= -> 4 ;
                //+, - -> 5 ; *, /, % -> 6

                case TokenType::or_or: return 1;
                case TokenType::and_and: return 2;
                case TokenType::equal_equal:
                case TokenType::not_equal: return 3;
                case TokenType::less:
                case TokenType::less_equal:
                case TokenType::greater:
                case TokenType::greater_equal: return 4;
                case TokenType::plus:
                case TokenType::minus: return 5;
                case TokenType::star:
                case TokenType::slash:
                case TokenType::percent: return 6;
                default: return -1;
            }
        }

        static BinOp token_to_binop(TokenType t) {
            switch (t) {
                case TokenType::plus: return BinOp::Add;
                case TokenType::minus: return BinOp::Sub;
                case TokenType::star: return BinOp::Mul;
                case TokenType::slash: return BinOp::Div;
                case TokenType::percent: return BinOp::Mod;
                case TokenType::equal_equal: return BinOp::Eq;
                case TokenType::not_equal: return BinOp::Neq;
                case TokenType::less: return BinOp::Lt;
                case TokenType::greater: return BinOp::Gt;
                case TokenType::less_equal: return BinOp::Leq;
                case TokenType::greater_equal: return BinOp::Geq;
                case TokenType::and_and: return BinOp::And;
                case TokenType::or_or: return BinOp::Or;
                default: 
                    std::cerr << "Internal error: not a binary operator" << std::endl;
                    exit(EXIT_FAILURE);
            }
        }

        std::optional<NodeExpr> parse_primary() {

            if (peek().has_value() && peek().value().type == TokenType::not_op){
                consume();
                auto operand = parse_primary();
                if (!operand) {
                    std::cerr << "Expected operand after 'not'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                return NodeExpr{ .var = NodeUnaryExpr{ .op = UnaryOp::Not,
                                 .operand = std::make_shared<NodeExpr>(operand.value())}};
            }
            if (peek().has_value() && peek().value().type == TokenType::minus) {
                consume();
                auto operand = parse_primary();
                if (!operand) {
                    std::cerr << "Expected operand after '-'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                return NodeExpr{ .var = NodeUnaryExpr{ .op = UnaryOp::Neg,
                                 .operand = std::make_shared<NodeExpr>(operand.value())}};
            }
            if (peek().has_value() && peek().value().type == TokenType::open_paren) {
                consume();
                auto expr = parse_expr();
                if (!expr) {
                    std::cerr << "Expected expression inside '()'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                    std::cerr << "Expected ')' after expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                consume();
                return expr;
            }

            if (peek().has_value() && peek().value().type == TokenType::integer) {
                return NodeExpr{ .var = NodeInteger { .integer = consume()}};
            }

            if (peek().has_value() && peek().value().type == TokenType::identifier) {
                return NodeExpr{ .var = NodeIdentifier { .identifier = consume()}};
            }

            return {};
        }

        std::optional<NodeExpr> parse_expr(int min_prec = 0) {
            auto lhs = parse_primary();
            if (!lhs) return {};

            while (peek().has_value() && bin_prec(peek().value().type) >= min_prec) {
                Token op_tok = consume();
                int next_prec = bin_prec(op_tok.type) + 1;
                auto rhs = parse_expr(next_prec);
                if (!rhs) {
                    std::cerr << "Expected expression after operator" << std::endl;
                    exit(EXIT_FAILURE);
                }
                NodeBinExpr bin;
                bin.op = token_to_binop(op_tok.type);
                bin.left = std::make_shared<NodeExpr>(lhs.value());
                bin.right = std::make_shared<NodeExpr>(rhs.value());
                lhs = NodeExpr{ .var = bin };
            }
            return lhs;
        }

        std::optional<NodeStmt> parse_stmt() {
            if (peek().has_value() && peek().value().type == TokenType::exit
                && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
                consume();
                consume();
                auto expr = parse_expr();
                if (!expr) {
                        std::cerr << "Expected expression inside getback()" << std::endl;
                    exit(EXIT_FAILURE);
                    }
                    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                        std::cerr << "Expected ')' after expression in getback()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume(); // consume ')'
                    if (!peek().has_value() ||
                        peek().value().type != TokenType::semicolon) {
                        std::cerr << "Expected ';' after getback()\n";
                        exit(EXIT_FAILURE);
                    }

                    consume(); // consume ';'

                    return NodeStmt{ .var = NodeExit{ .expr = expr.value() }};
            }

            if (peek().has_value() && peek().value().type == TokenType::assume 
                && peek(1).has_value() && peek(1).value().type == TokenType::identifier
                && peek(2).has_value() && peek(2).value().type == TokenType::equal) {
                    consume();
                    auto stmt_assume = NodeAssume{ .identifier = consume() };
                    consume();

                    auto expr = parse_expr();

                    if (!expr) {
                        std::cerr << "Expected expression after '='\n";
                        exit(EXIT_FAILURE);
                    }

                    stmt_assume.expr = expr.value();

                    if (!peek().has_value() ||
                        peek().value().type != TokenType::semicolon) {
                        std::cerr << "Expected ';'\n";
                        exit(EXIT_FAILURE);
                    }

                    consume();

                    return NodeStmt{ .var = stmt_assume };
                }
                return {};
        }

        std::optional<NodeProg> parse_prog() {
            NodeProg prog;
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    prog.stmts.push_back(stmt.value());
                } else {
                    std::cerr << "Unexpected token: " << peek().value().value.value_or("") << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return prog;   
        }

    private:
        [[nodiscard]] inline std::optional<Token> peek(size_t offset = 0) const {
            if (m_index + offset >= m_tokens.size())
                 return {};

            return m_tokens[m_index + offset];
        }

        inline Token consume() { return m_tokens.at(m_index++); }

        const std::vector<Token> m_tokens;
        size_t m_index;

        
};
