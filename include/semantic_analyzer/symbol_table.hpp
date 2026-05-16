#pragma once
#include "parser/type.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
namespace llvm {
class Value;
}
struct SymbolInfo {
  Type type;
  bool is_const;
  bool is_function;
  bool is_extern;
  std::vector<Type> param_types;
  llvm::Value *llvm_memory_address;
  SymbolInfo()
      : type(Type::from_type_node(TypeNode::INT)), is_const(false),
        is_function(false), is_extern(false), llvm_memory_address(nullptr) {}
  SymbolInfo(Type t, bool is_const)
      : type(t), is_const(is_const), is_function(false), is_extern(false),
        llvm_memory_address(nullptr) {}
  SymbolInfo(Type ret_type, std::vector<Type> param_types,
             bool is_extern = false)
      : type(ret_type), is_const(false), is_function(true),
        is_extern(is_extern), param_types(param_types),
        llvm_memory_address(nullptr) {}
};
struct SymbolTable {
  std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;
  SymbolTable() { enter_scope(); }
  void enter_scope();
  void exit_scope();
  bool declare(const std::string &name, SymbolInfo info);
  SymbolInfo *lookup(const std::string &name);
};
inline void SymbolTable::enter_scope() { scopes.emplace_back(); }
inline void SymbolTable::exit_scope() {
  if (scopes.size() <= 1) {
    std::cerr << "Compiler Bug: Scope Underflow\n";
    std::exit(1);
  }
  scopes.pop_back();
}
inline bool SymbolTable::declare(const std::string &name, SymbolInfo info) {
  auto &current_scope = scopes.back();
  if (current_scope.find(name) != current_scope.end()) {
    return false;
  }
  current_scope[name] = info;
  return true;
}
inline SymbolInfo *SymbolTable::lookup(const std::string &name) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); it++) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}
