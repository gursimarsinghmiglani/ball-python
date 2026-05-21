#pragma once
#include "parser/ast.hpp"
#include "semantic_analyzer/symbol_table.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
class CodeGen {
public:
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unique_ptr<llvm::Module> module;
  SymbolTable st;
  llvm::Function *current_function = nullptr;
  llvm::Function *global_init_func = nullptr;
  std::vector<std::function<void()>> global_init_instructions;
  CodeGen();
  void generate(AST *root);
  void optimize();
  void emit_object_code(const std::string &filename);
  void dump_ir();
private:
  llvm::Type *get_llvm_type(const Type &type);
  llvm::Value *visit(AST *node);
  llvm::Value *visit_program(AST *node);
  llvm::Value *visit_decl(AST *node);
  llvm::Value *visit_var_decl(AST *node);
  llvm::Value *visit_const_decl(AST *node);
  llvm::Value *visit_function_decl(AST *node);
  llvm::Value *visit_block(AST *node);
  llvm::Value *visit_if_stmt(AST *node);
  llvm::Value *visit_while_stmt(AST *node);
  llvm::Value *visit_return_stmt(AST *node);
  llvm::Value *visit_expr_stmt(AST *node);
  llvm::Value *visit_expr(AST *node);
  llvm::Value *visit_primary_expr(AST *node);
  llvm::Value *visit_assign_expr(AST *node);
  llvm::Value *visit_binary_expr(AST *node);
  llvm::Value *visit_postfix_expr(AST *node);
  llvm::Value *visit_print_stmt(AST *node);
  llvm::Value *visit_for_stmt(AST *node);
  llvm::Value *visit_unary_expr(AST *node);
  llvm::AllocaInst *create_entry_block_alloca(llvm::Function *function,
                                              const std::string &var_name,
                                              llvm::Type *type);
  llvm::Value *allocate_tensor_on_stack(const Type &type, const std::string &name = "tensor");
};