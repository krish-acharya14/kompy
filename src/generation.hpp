#pragma once

#include "parser.hpp"
#include <string>
#include <unordered_map>

class Generator {
    public:
        inline explicit Generator(NodeProg prog) : m_prog(std::move(prog)) {}

        void gen_expr(const NodeExpr& expr) {
            struct ExprVisitor {
                Generator* gen;

                void operator()(const NodeInteger& e) const {
                    gen->m_output << "    mov rax, " << e.integer.value.value() << "\n";
                    gen->push("rax");
                }

                void operator()(const NodeIdentifier& e) const {
                    auto it = gen->m_vars.find(e.identifier.value.value());
                    if (it == gen->m_vars.end()) {
                        std::cerr << "Undefined variable: " << e.identifier.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    offset << "QWORD [rsp + " << (gen->m_stack_size - it->second.stack_loc - 1) * 8 << "]";
                    gen->push(offset.str());
                }

                void operator()(const NodeUnaryExpr& e) const {
                    gen->gen_expr(*e.operand);
                    gen->pop("rax");
                    switch (e.op) {
                        case UnaryOp::Not:
                            gen->m_output << "    test rax, rax\n";
                            gen->m_output << "    sete al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case UnaryOp::Neg:
                            gen->m_output <<"    neg rax\n";
                            break;
                    }
                    gen->push("rax");
                }

                void operator()(const NodeBinExpr& e) const {
                    gen->gen_expr(*e.left);
                    gen->gen_expr(*e.right);
                    gen->pop("rbx");          //right operand
                    gen->pop("rax");          //left operand
                    switch (e.op) {
                        // Arithmetic
                        case BinOp::Add:
                            gen->m_output << "    add rax, rbx\n";
                            break;
                        case BinOp::Sub:
                            gen->m_output << "    sub rax, rbx\n";
                            break;
                        case BinOp::Mul:
                            gen->m_output << "     imul rax, rbx\n";
                            break;
                        case BinOp::Div:
                            gen->m_output << "    cqo\n";
                            gen->m_output << "    idiv rbx\n";
                            break;
                        case BinOp::Mod:
                            gen->m_output << "    cqo\n";
                            gen->m_output << "    idiv rbx\n";
                            gen->m_output << "    mov rax, rdx\n";
                            break;

                        // Comparison
                        case BinOp::Eq:
                            gen->m_output << "    cmp rax, rbx\n";
                            gen->m_output << "    sete al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Neq:
                            gen->m_output << "    cmp rax, rbx\n";
                            gen->m_output << "    setne al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Lt:
                            gen->m_output << "    cmp rax, rbx\n";
                            gen->m_output << "    setl al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Gt:
                            gen->m_output << "    cmp rax, rbx\n";
                            gen->m_output << "    setg al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Leq:
                            gen->m_output << "    cmp rax, rbx\n";
                            gen->m_output << "    setle al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Geq:
                            gen->m_output << "    cmp rax, rbx\n";
                            gen->m_output << "    setge al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;

                        // Logic
                        case BinOp::And:
                            gen->m_output << "    test rax, rax\n";
                            gen->m_output << "    setne al\n";
                            gen->m_output << "    test rbx, rbx\n";
                            gen->m_output << "    setne bl\n";
                            gen->m_output << "    and al, bl\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Or:
                            gen->m_output << "    test rax, rax\n";
                            gen->m_output << "    setne al\n";
                            gen->m_output << "    test rbx, rbx\n";
                            gen->m_output << "    setne bl\n";
                            gen->m_output << "    or al, bl\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                    }
                    gen->push("rax");   
                }
            };
            std::visit(ExprVisitor{ .gen = this }, expr.var);
        }

        void gen_stmt(const NodeStmt& stmt) {
        struct StmtVisitor {
            Generator* gen;

            void operator()(const NodeAssume& s) const {
                if (gen->m_vars.count(s.identifier.value.value())) {
                    std::cerr << "Identifier already used: " << s.identifier.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen->m_vars.insert({ s.identifier.value.value(), Var{ .stack_loc = gen->m_stack_size } });
                gen->gen_expr(s.expr);
            }

            void operator()(const NodeExit& s) const {
                gen->gen_expr(s.expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }
        };

        std::visit(StmtVisitor{ .gen = this }, stmt.var);
    }

        [[nodiscard]] std::string gen_prog() {
            m_output << "global _start\n";
            m_output << "section .text\n";
            m_output << "_start:\n";
            for (const NodeStmt& stmt : m_prog.stmts) 
                gen_stmt(stmt);
            
            m_output << "    mov rax, 60\n";
            m_output << "    mov rdi, 0\n";
            m_output << "    syscall\n";
            return m_output.str();
        }

    private:
        void push(const std::string& reg) {
            m_output << "    push " << reg << "\n";
            m_stack_size++;
        }
        void pop(const std::string& reg) {
            m_output << "    pop " << reg << "\n";
            m_stack_size--;
        }

        struct Var { size_t stack_loc; };

        const NodeProg m_prog;
        std::unordered_map<std::string, Var> m_vars;
        std::stringstream m_output;
        size_t m_stack_size = 0;
};
