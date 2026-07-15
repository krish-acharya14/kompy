#pragma once

#include "parser.hpp"
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Generator {
    public:
        inline explicit Generator(NodeProg prog) : m_prog(std::move(prog)) {}

        // Expression codegen

        void gen_expr(const NodeExpr& expr) {
            struct ExprVisitor {
                Generator* gen;

                void operator()(const NodeInteger& e) const {
                    gen->m_output << "    mov rax, " << e.integer.value.value() << "\n";
                    gen->push("rax");
                }
                void operator()(const NodeIdentifier& e) const {
                    const std::string& name = e.identifier.value.value();
                    for (int i = (int)gen->m_scopes.size() - 1; i >= 0; --i) {
                        auto it = gen->m_scopes[i].find(name);
                        if (it != gen->m_scopes[i].end()) {
                            std::stringstream ss;
                            ss << "QWORD [rsp + " << (gen->m_stack_size - it->second.stack_loc - 1) * 8 << "]";
                            gen->push(ss.str());
                            return;
                        }
                    }
                    std::cerr << "Undefined variable: " << name << std::endl;
                    exit(EXIT_FAILURE);
                }

                void operator()(const NodeUnary& e) const {
                    gen->gen_expr(*e.operand);
                    gen->pop("rax");
                    switch (e.op) {
                        case UnaryOp::Not:
                            gen->m_output << "    test rax, rax\n";
                            gen->m_output << "    sete al\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case UnaryOp::Neg:
                            gen->m_output << "    neg rax\n";
                            break;
                    }
                    gen->push("rax");
                }

                void operator()(const NodeBinary& e) const {
                    gen->gen_expr(*e.left);
                    gen->gen_expr(*e.right);
                    gen->pop("rbx");
                    gen->pop("rax");
                    switch (e.op) {
                        case BinOp::Add:
                            gen->m_output << "    add rax, rbx\n";
                            break;
                        case BinOp::Sub:
                            gen->m_output << "    sub rax, rbx\n";
                            break;
                        case BinOp::Mul:
                            gen->m_output << "    imul rax, rbx\n";
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
                        case BinOp::And:
                            gen->m_output << "    test rax, rax\n";
                            gen->m_output << "    sete al\n";
                            gen->m_output << "    test rbx, rbx\n";
                            gen->m_output << "    sete bl\n";
                            gen->m_output << "    and al, bl\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                        case BinOp::Or:
                            gen->m_output << "    test rax, rax\n";
                            gen->m_output << "    sete al\n";
                            gen->m_output << "    test rbx, rbx\n";
                            gen->m_output << "    sete bl\n";
                            gen->m_output << "    or al, bl\n";
                            gen->m_output << "    movzx rax, al\n";
                            break;
                    }
                    gen->push("rax");
                }

                void operator()(const NodeCall e) const {
                    gen->gen_call(e);
                }
            };
            std::visit(ExprVisitor{ .gen = this }, expr.var);
        }

        // Function call
        void gen_call(const NodeCall& call) {
            const std::string& name = call.identifier.value.value();

            auto it = m_functions.find(name);
            if (it == m_functions.end()) {
                for (int i = (int)m_scopes.size() - 1; i >= 0; --i) {
                    if (m_scopes[i].count(name)) {
                        std::cerr << "Attempt to call non-function '" << name << "'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                std::cerr << "Undefined function:" << name << std::endl;
                exit(EXIT_FAILURE);
            }

            const char* arg_regs[6] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
            size_t n = call.args.size();

            if (n > 6) {
                for (size_t i = n; i >= 6; --i) 
                    gen_expr(*call.args[i]);
            }

            size_t first = (n < 6) ? n : 6;
            for (size_t i = 0; i < first; i++) 
                gen_expr(*call.args[i]);

            for (size_t i = first; i-- > 0; )
                pop(arg_regs[i]);

            m_output << "    call fn_" << name << "\n";

            if (n > 6) {
                size_t extra = (n - 6) * 8;
                m_output << "    add rsp, " << extra << "\n";
                m_stack_size -= (n - 6);
            }
            push("rax");
        }

        size_t begin_scope() {
            m_scopes.push_back({});
            return m_stack_size;
        }

        void end_scope(size_t stack_size_before) {
            size_t vars_in_scope = m_stack_size - stack_size_before;
            if (vars_in_scope > 0) {
                m_output << "    add rsp, " << vars_in_scope * 8 << "\n";
                m_stack_size -= vars_in_scope;
            }
            m_scopes.pop_back();
        }

        void declare_var(const std::string& name) {
            if (m_scopes.back().count(name)) {
                std::cerr << "Variable already declared in this scope: " << name << std::endl;
                exit(EXIT_FAILURE);
            }
            m_scopes.back().insert({ name, Var{ .stack_loc = m_stack_size - 1 } });
        }

        void gen_stmt(const NodeStmt& stmt) {
            struct StmtVisitor {
                Generator* gen;

                void operator()(const NodeExit& s) const {
                    gen->gen_expr(s.expr);
                    gen->m_output << "    mov rax, 60\n";
                    gen->pop("rdi");
                    gen->m_output << "    syscall\n";
                }

                void operator()(const NodeAssume& s) const {
                    gen->gen_expr(s.expr);
                    gen->declare_var(s.identifier.value.value());
                }

                void operator()(const NodeAssign& s) const {
                    gen->gen_expr(s.expr);

                    gen->pop("rax");

                    for (int i = (int)gen->m_scopes.size() - 1; i >= 0; --i) {
                        auto it = gen->m_scopes[i].find(s.identifier.value.value());

                        if (it != gen->m_scopes[i].end()) {

                            gen->m_output << "    mov QWORD [rsp + " << (gen->m_stack_size - it->second.stack_loc - 1) * 8 << "], rax\n";

                            return;
                        }
                    }

                    std::cerr << "Undefined variable: "
                            << s.identifier.value.value()
                            << std::endl;

                    exit(EXIT_FAILURE);
                }

                void operator()(const NodeBlock& s) const {
                    size_t saved = gen->begin_scope();
                    for (const auto& stmt_ptr : s.stmts) 
                        gen->gen_stmt(*stmt_ptr);
                    gen->end_scope(saved);
                }

                void operator()(const NodeMaybe& s) const {
                    size_t lbl = gen->m_label_count++;

                    gen->gen_expr(s.condn);
                    gen->pop("rax");
                    gen->m_output << "    test rax, rax\n";
                    if (s.else_block.has_value()) {
                        gen->m_output << "    jz .else_" << lbl << "\n";

                        size_t saved_then = gen->begin_scope();
                        for (const auto& stmt_ptr : s.then_block.stmts)
                            gen->gen_stmt(*stmt_ptr);
                        
                        gen->end_scope(saved_then);
                        gen->m_output << "    jmp .end_" << lbl << "\n";

                        gen->m_output << ".else_" << lbl << ":\n";
                        gen->gen_stmt(*s.else_block.value());
                    } else {
                        gen->m_output << "    jz .end_" << lbl << "\n";

                        size_t saved = gen->begin_scope();
                        for (const auto& stmt_ptr : s.then_block.stmts)
                            gen->gen_stmt(*stmt_ptr);
                        gen->end_scope(saved);
                    }
                    gen->m_output << ".end_" << lbl << ":\n";
                }

                void operator()(const NodeWhile& s) const {
                    size_t lbl = gen->m_label_count++;

                    gen->m_output << ".loop_start_" << lbl << ":\n";
                    gen->gen_expr(s.condn);
                    gen->pop("rax");
                    gen->m_output << "    test rax, rax\n";
                    gen->m_output << "    jz .loop_end_" << lbl << "\n";

                    size_t saved = gen->begin_scope();
                    for (const auto& stmt_ptr : s.body.stmts)
                        gen->gen_stmt(*stmt_ptr);
                    gen->end_scope(saved);

                    gen->m_output << "    jmp .loop_start_" << lbl << "\n";
                    gen->m_output << ".loop_end_" << lbl << ":\n";
                }

                void operator()(const NodeReturn& s) const {
                    if (!gen->m_in_function) {
                        std::cerr << "'return used outside of a function" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    
                    gen->gen_expr(s.expr);
                    gen->pop("rax");
                    gen->m_output << "    jmp " << gen->m_current_ret_label << "\n";
                }

                void operator()(const NodeCallStmt& s) const {
                    gen->gen_call(s.call);
                    gen->pop("rax");
                }
            };
            std::visit(StmtVisitor{ .gen = this }, stmt.var);
        }

        // Function codegen
        void gen_function(const NodeFunction& fn) {
            const std::string& fname = fn.name.value.value();

            m_scopes.clear();
            m_stack_size = 0;
            m_in_function = true;

            std::stringstream ret_lbl;
            ret_lbl << ".ret_fn_" << m_label_count++;
            m_current_ret_label = ret_lbl.str();

            m_output << "fn_" << fname << ":\n";
            m_output << "    push rbp\n";
            m_output << "    mov rbp, rsp\n";

            m_stack_size = 1;
            begin_scope();

            std::unordered_set<std::string> seen_param;
            const char* arg_regs[6] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
            for (size_t i = 0; i < fn.params.size(); ++i) {
                const std::string& pname = fn.params[i].value.value();
                if (!seen_param.insert(pname).second) {
                    std::cerr << "Duplicate parameter '" << pname << "' in function '" << fname << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (i < 6) push(arg_regs[i]);
                else {
                    m_output << "    mov rax, QWORD [rbp + " << (16 + (i - 6) * 8) << "]\n";
                    push("rax");
                }
                declare_var(pname);
            }

            for (const auto& stmt_ptr : fn.body.stmts) 
                gen_stmt(*stmt_ptr);
            
            m_output << "    xor rax, rax\n";
            m_output << m_current_ret_label << ":\n";
            m_output << "    mov rsp, rbp\n";
            m_output << "    pop rbp\n";
            m_output << "    ret\n";

            m_in_function = false;
            m_current_ret_label.clear();
            m_scopes.clear();
            m_stack_size = 0;
        }

        [[nodiscard]] std::string gen_prog() {
            for (const auto& fn : m_prog.functions) {
                const std::string& fname = fn.name.value.value();
                if (!m_functions.emplace(fname, fn.params.size()).second) {
                    std::cerr << "Duplicate function definition: '" << fname << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }

            m_output << "global _start\n";
            m_output << "section .text\n";
            m_output << "_start:\n";

            m_in_function = false;
            begin_scope();
            for (const NodeStmt& stmt : m_prog.stmts) 
                gen_stmt(stmt);
            
            m_output << "    mov rax, 60\n";
            m_output << "    xor rdi, rdi\n";
            m_output << "    syscall\n";

            for (const auto& fn : m_prog.functions) 
                gen_function(fn);
            
            return m_output.str();
        }
    
    private:
        void push(const std::string& src) {
            m_output << "    push " << src << "\n";
            m_stack_size++;
        }
        void pop(const std::string& dst) {
            m_output << "    pop " << dst << "\n";
            m_stack_size--;
        }

        struct Var { size_t stack_loc; };

        std::vector<std::unordered_map<std::string, Var>> m_scopes;

        std::unordered_map<std::string, size_t> m_functions;

        bool m_in_function = false;
        std::string m_current_ret_label;

        const NodeProg m_prog;
        std::stringstream m_output;
        size_t m_stack_size = 0;
        size_t m_label_count = 0;
};