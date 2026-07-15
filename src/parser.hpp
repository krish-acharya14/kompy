#pragma once

#include <memory>
#include <variant>
#include "tokenization.hpp"

struct NodeStmt;

struct NodeExpr;

enum class BinOp {
    Add, Sub, Mul, Div, Mod,
    Eq, Neq, Lt, Gt, Leq, Geq,
    And, Or,
};

enum class UnaryOp {
    Neg, Not,
};

struct NodeInteger {
    Token integer;
};

struct NodeIdentifier {
    Token identifier;
};

struct NodeBinary {
    BinOp op;
    std::shared_ptr<NodeExpr> left;
    std::shared_ptr<NodeExpr> right;
};

struct NodeUnary {
    UnaryOp op;
    std::shared_ptr<NodeExpr> operand;
};

struct NodeCall {
    Token identifier;
    std::vector<std::shared_ptr<NodeExpr>> args;
};

struct NodeExpr {
    std::variant<NodeInteger, NodeIdentifier, NodeBinary, NodeUnary, NodeCall> var;
};

struct NodeExit {
    NodeExpr expr;
};

struct NodeAssume {
    Token identifier;
    NodeExpr expr;
};

struct NodeAssign {
    Token identifier;
    NodeExpr expr;
};

struct NodeBlock {
    std::vector<std::shared_ptr<NodeStmt>> stmts;
};

struct NodeMaybe {
    NodeExpr condn;
    NodeBlock then_block;
    std::optional<std::shared_ptr<NodeStmt>> else_block;
};

struct NodeWhile {
    NodeExpr condn;
    NodeBlock body;
};

struct NodeReturn {
    NodeExpr expr;
};

struct NodeCallStmt {
    NodeCall call;
};

struct NodeStmt {
    std::variant<NodeExit, NodeAssume, NodeAssign, NodeMaybe, NodeBlock, NodeWhile, NodeReturn, NodeCallStmt> var;
};

struct NodeFunction {
    Token name;
    std::vector<Token> params;
    NodeBlock body;
};

struct NodeProg {
    std::vector<NodeStmt> stmts;
    std::vector<NodeFunction> functions;
};

class Parser {
    public:
        inline explicit Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens)), m_index(0) {}

        static int bin_prec(TokenType t) {
            switch (t) {
                case TokenType::or_or:                                    return 1;
                case TokenType::and_and:                                  return 2;
                case TokenType::equal_equal: case TokenType::not_equal:   return 3;
                case TokenType::less:    case TokenType::less_equal:
                case TokenType::greater: case TokenType::greater_equal:   return 4;
                case TokenType::plus:  case TokenType::minus:             return 5;
                case TokenType::star:  case TokenType::slash:
                case TokenType::percent:                                  return 6;
                default: return -1;
            }
        }

        static BinOp token_to_binop(TokenType t) {
            switch (t) {
                case TokenType::plus:          return BinOp::Add;
                case TokenType::minus:         return BinOp::Sub;
                case TokenType::star:          return BinOp::Mul;
                case TokenType::slash:         return BinOp::Div;
                case TokenType::percent:       return BinOp::Mod;
                case TokenType::equal_equal:   return BinOp::Eq;
                case TokenType::not_equal:     return BinOp::Neq;
                case TokenType::less:          return BinOp::Lt;
                case TokenType::greater:       return BinOp::Gt;
                case TokenType::less_equal:    return BinOp::Leq;
                case TokenType::greater_equal: return BinOp::Geq;
                case TokenType::and_and:       return BinOp::And;
                case TokenType::or_or:         return BinOp::Or;
                default:
                    std::cerr << "Internal error: not a binary operator" << std::endl;
                    exit(EXIT_FAILURE);
            }
        }

        // function call parsing
        NodeCall parse_call(Token ident_tok) {
            if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
                std::cerr << "Expected '(' in fuction call to '" << ident_tok.value.value_or("?") << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
            consume();

            NodeCall call;
            call.identifier = ident_tok;

            if (peek().has_value() && peek().value().type == TokenType::close_paren) {
                consume();
                return call;
            }

            while (true) {
                auto arg = parse_expr();
                if (!arg) {
                    std::cerr << "Expected argument expression in call to '" << ident_tok.value.value_or("?") << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                call.args.push_back(std::make_shared<NodeExpr>(arg.value()));

                if (peek().has_value() && peek().value().type == TokenType::comma) {
                    consume();
                    continue;
                }
                break;
            }

            if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                std::cerr << "Expected ')' after arguments call in call to '" << ident_tok.value.value_or("?") << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
            consume();

            return call;
        }

        std::optional<NodeExpr> parse_primary() {

            // Unary Operator
            if (peek().has_value() && peek().value().type == TokenType::not_op) {
                consume();
                auto operand = parse_primary();
                if (!operand) { 
                    std::cerr << "Expected operand after '!' " << std::endl;
                    exit(EXIT_FAILURE);
                }
                return NodeExpr{ .var = NodeUnary { .op = UnaryOp::Not, .operand = std::make_shared<NodeExpr>(operand.value())}};
            }
            if (peek().has_value() && peek().value().type == TokenType::minus) {
                consume();
                auto operand = parse_primary();
                if (!operand) {
                    std::cerr << "Expected operand after '-'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                return NodeExpr{ .var = NodeUnary { .op = UnaryOp::Neg, .operand = std::make_shared<NodeExpr>(operand.value())}};
            }

            // Parenthesised expression 
            if (peek().has_value() && peek().value().type == TokenType::open_paren) {
                consume();
                auto expr = parse_expr();
                if (!expr) {
                    std::cerr << "Expected expression inside '()'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if(!peek().has_value() || peek().value().type != TokenType::close_paren) {
                    std::cerr << "Expected ')' after expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                consume();
                return expr;
            }

            // Integer literal
            if (peek().has_value() && peek().value().type == TokenType::integer) {
                return NodeExpr{ .var = NodeInteger{ .integer = consume() }};
            }

            // Identifier or Function call
            if (peek().has_value() && peek().value().type == TokenType::identifier) {
                Token ident_tok = consume();

                if (peek().has_value() && peek().value().type == TokenType::open_paren) {
                    NodeCall call = parse_call(ident_tok);
                    return NodeExpr{ .var = call };
                }
                return NodeExpr{ .var = NodeIdentifier{ .identifier = ident_tok }};
            }

            return {};
        }

        std::optional<NodeExpr> parse_expr(int min_prec = 0) {
            auto lhs = parse_primary();
            if (!lhs) return {};

            while (peek().has_value() && bin_prec(peek().value().type) >= min_prec) {
                Token op_tok = consume();
                auto rhs = parse_expr(bin_prec(op_tok.type) + 1);
                if (!rhs) {
                    std::cerr << "Expected expression after operator" << std::endl;
                    exit(EXIT_FAILURE);
                }
                NodeBinary bin;
                bin.op = token_to_binop(op_tok.type);
                bin.left = std::make_shared<NodeExpr>(lhs.value());
                bin.right = std::make_shared<NodeExpr>(rhs.value());
                lhs = NodeExpr{ .var = bin };
            }
            return lhs;
        }
        
        // Block parsing
        NodeBlock parse_block() {
            if (!peek().has_value() || peek().value().type != TokenType::open_brace) {
                std::cerr << "Expected '{'" << std::endl;
                exit(EXIT_FAILURE);
            }
            consume();

            NodeBlock block;
            while (peek().has_value() && peek().value().type != TokenType::close_brace) {
                auto stmt = parse_stmt();
                if (!stmt) {
                    std::cerr << "Unexpected token inside block: " << peek().value().value.value_or("?") << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                block.stmts.push_back(std::make_shared<NodeStmt>(stmt.value()));
            }
            
            if (!peek().has_value()) {
                std::cerr << "Expected '}' to close block" << std::endl;
                exit(EXIT_FAILURE);
            }
            consume();
            return block;
        }

        // Statement parsing

        std::optional<NodeStmt> parse_stmt() {

            // getback(expr);
            if (peek().has_value() && peek().value().type == TokenType::exit
                && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
                    consume();
                    consume();
                    auto expr = parse_expr();
                    if (!expr) {
                        std::cerr << "Expected expression inside getback()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    // consume();
                    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                        std::cerr << "Expected ')' after expression in getback()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();
                    if (!peek().has_value() || peek().value().type != TokenType::semicolon) {
                        std::cerr << "Expected ';' after getback()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();
                    return NodeStmt{ .var = NodeExit{ .expr = expr.value()}};
                }

                // return(expr);
                if (peek().has_value() && peek().value().type == TokenType::ret) {
                    consume();
                    // consume();
                    if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
                        std::cerr << "Expected '(' after 'return'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    auto expr = parse_expr();
                    if (!expr) {
                        std::cerr << "Expected expression inside return()" << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                        std::cerr << "Expected ')' after expression in return()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    if (!peek().has_value() || peek().value().type != TokenType::semicolon) {
                        std::cerr << "Expected ';' after return()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    return NodeStmt{ .var = NodeReturn{ .expr = expr.value() }};
                }
            
                // assume x = expr;
                if (peek().has_value() && peek().value().type == TokenType::assume
                    && peek(1).has_value() && peek(1).value().type == TokenType::identifier
                    && peek(2).has_value() && peek(2).value().type == TokenType::equal) {
                        consume();
                        auto stmt_assume = NodeAssume{ .identifier = consume() };
                        consume();
                        auto expr = parse_expr();
                        if (!expr) {
                            std::cerr << "Expected expression after '='" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        stmt_assume.expr = expr.value();
                        if (!peek().has_value() || peek().value().type != TokenType::semicolon) {
                            std::cerr << "Expected ';' after assume statement" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        consume();
                        return NodeStmt{ .var = stmt_assume };
                }

                // identifier = expr;
                if (peek().has_value() && peek().value().type == TokenType::identifier && peek(1).has_value() && peek(1).value().type == TokenType::equal) {
                    NodeAssign stmt;

                    stmt.identifier = consume();
                    consume();

                    auto expr = parse_expr();

                    if (!expr) {
                        std::cerr << "Expected expression after '='" << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    stmt.expr = expr.value();

                    if (!peek().has_value() || peek().value().type != TokenType::semicolon) {
                        std::cerr << "Expected ';' after assignment" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    return NodeStmt{ .var = stmt };
                }

                // identifier(args);
                if (peek().has_value() && peek().value().type == TokenType::identifier 
                    && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
                        Token ident_tok = consume();
                        NodeCall call = parse_call(ident_tok);

                        if (!peek().has_value() || peek().value().type != TokenType::semicolon) {
                            std::cerr << "Expected ';' after call to '" << ident_tok.value.value_or("?") << "'" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        consume();

                        return NodeStmt{ .var = NodeCallStmt{ .call = call }};
                }

                // {Block}
                if (peek().has_value() && peek().value().type == TokenType::open_brace) {
                    return NodeStmt{ .var = parse_block() };
                }

                // Conditional Statements
                if (peek().has_value() && peek().value().type == TokenType::maybe) {
                    consume();

                    if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
                        std::cerr << "Expected '(' after 'maybe'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();
                    auto condn = parse_expr();
                    if (!condn) {
                        std::cerr << "Expected condition expression in 'maybe'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                        std::cerr << "Expected ')' after condition in 'maybe'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    NodeMaybe node_maybe;
                    node_maybe.condn = condn.value();
                    node_maybe.then_block = parse_block();

                    if (peek().has_value() && peek().value().type == TokenType::otherwise) {
                        consume();

                        if (peek().has_value() && peek().value().type == TokenType::maybe) {
                            auto chained = parse_stmt();
                            if (!chained) {
                                std::cerr << "Expected maybe or block after otherwise" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            node_maybe.else_block = std::make_shared<NodeStmt>(chained.value());
                        } else {
                            NodeBlock elseBlk = parse_block();
                            node_maybe.else_block = std::make_shared<NodeStmt>(NodeStmt{ .var = std::move(elseBlk) });
                        }
                    }
                    return NodeStmt{ .var = node_maybe };
                }

                // while (condn) { ... }
                if (peek().has_value() && peek().value().type == TokenType::while_loop) {
                    consume();

                    if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
                        std::cerr << "Expected '(' after 'while'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    auto condn = parse_expr();
                    if (!condn) {
                        std::cerr << "Expected condition expression in 'while'" << std::endl;
                        exit(EXIT_FAILURE);
                    }

                    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                        std::cerr << "Expected ')' after condition in 'while'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    consume();

                    NodeWhile node_while;
                    node_while.condn = condn.value();
                    node_while.body = parse_block();
                    return NodeStmt{ .var = node_while };
                }
                return {};
            }

            // fn name(p1, p2, ...) {...}
            NodeFunction parse_function() {
                consume();

                if (!peek().has_value() || peek().value().type != TokenType::identifier) {
                    std::cerr << "Expected function name after 'fn'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                NodeFunction fn;
                fn.name = consume();

                if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
                    std::cerr << "Expected '(' after function name '" << fn.name.value.value_or("?") << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                consume();

                // Param list
                if (peek().has_value() && peek().value().type != TokenType::close_paren) {
                    while (true) {
                        if (!peek().has_value() || peek().value().type != TokenType::identifier) {
                            std::cerr << "Expected parameter name in function '" << fn.name.value.value_or("?") << "'" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        fn.params.push_back(consume());
                        if (peek().has_value() && peek().value().type == TokenType::comma) {
                            consume();
                            continue;
                        }
                        break;
                    }
                }

                if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
                    std::cerr << "Expecte ')' after parameter list in function '" << fn.name.value.value_or("?") << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                consume();

                fn.body = parse_block();
                return fn;
            }

            std::optional<NodeProg> parse_prog() {
                NodeProg prog;
                while (peek().has_value()) {
                        if (peek()->type == TokenType::fn) {
                        prog.functions.push_back(parse_function());
                        continue;
                    }

                    auto stmt = parse_stmt();
                    if (!stmt) {
                        if (peek().has_value()) {
                            std::cerr << "Unexpected token '" << peek()->value.value_or("?") << "'" << std::endl;
                        } else {
                            std::cerr << "Unexpected end of file" << std::endl;
                        }
                        exit(EXIT_FAILURE);
                    }

                    prog.stmts.push_back(stmt.value());
                }
                return prog;
            }   
    
    private:
        [[nodiscard]] std::optional<Token> peek(size_t offset = 0) const {
            if (m_index + offset < m_tokens.size())
                return m_tokens[m_index + offset];
            return {};
        }

        inline Token consume() { return m_tokens.at(m_index++);}

        const std::vector<Token> m_tokens;
        size_t m_index;
};
