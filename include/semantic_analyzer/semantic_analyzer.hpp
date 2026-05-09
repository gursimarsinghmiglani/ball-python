#pragma once
#include "parser/ast.hpp"
#include "parser/type.hpp"
#include "symbol_table.hpp"
#include <iostream>
struct SemanticAnalyzer {
  std::unique_ptr<SymbolTable> st;
  std::optional<TypeNode> curr_func_ret_type;
  SemanticAnalyzer(AST *const root) { visit_program(root); }
  void error(AST *const root) {
    std::cerr << "Semantic error in line " << root->lexeme.line_number << " at position " << root->lexeme.start;
  }
  [[noreturn]] void type_error(AST *const root) {
    std::cerr << "Type mismatch error in line " << root->lexeme.line_number << " at position " << root->lexeme.start;
    exit(1);
  }
  void visit_program(AST *const node);
  void visit_decl(AST *const node);
  void visit_var_decl(AST *const node);
  static Type type_unify(const Type& type_left, const Type& type_right);
  static Type base_type_node_unify(TypeNode left, TypeNode right);
  static Type container_type_unify(const Type& left, const Type& right);
  static Type super_type_unify(const Type& left, const Type& right);
  static Type super_base_type_unify(TypeNode left, TypeNode right);
  static Type super_container_type_unify(const Type& left, const Type& right);
  void visit_id(AST *const node);
  void visit_expr(AST *const node);
  void visit_assign_expr(AST *const node);
  void visit_range_expr(AST *const node);
  void visit_logical_or_expr(AST *const node);
  void visit_logical_and_expr(AST *const node);
  void visit_bitwise_or_expr(AST *const node);
  void visit_bitwise_xor_expr(AST *const node);
  void visit_bitwise_and_expr(AST *const node);
  void visit_equality_expr(AST *const node);
  void visit_relational_expr(AST *const node);
  void visit_additive_expr(AST *const node);
  void visit_multiplicative_expr(AST *const node);
  void visit_unary_expr(AST *const node);
  void visit_postfix_expr(AST *const node);
  void visit_primary_expr(AST *const node);
  void visit_const_decl(AST *const node);
  void visit_function_decl(AST *const node);
  void visit_param(AST *const node);
  void visit_block(AST *const node);
  void visit_if_stmt(AST *const node);
  void visit_while_stmt(AST *const node);
  void visit_for_stmt(AST *const node);
  void visit_return_stmt(AST *const node);
  void visit_print_stmt(AST *const node);
  void visit_expr_stmt(AST *const node);
  void visit_extern_fn_decl(AST *const node);
};
inline void SemanticAnalyzer::visit_program(AST *const node) {
  for (const auto &child : node->children) {
    visit_decl(child.get());
  }
}
inline void SemanticAnalyzer::visit_decl(AST *const node) {
  switch (node->node) {
  case Node::VAR_DECL:
    visit_var_decl(node);
    break;
  case Node::CONST_DECL:
    visit_const_decl(node);
    break;
  case Node::FUNCTION_DECL:
    visit_function_decl(node);
    break;
  case Node::EXTERN_DECL:
    visit_extern_fn_decl(node);
    break;
  default:
    break;
  }
}
inline void SemanticAnalyzer::visit_var_decl(AST *const node) {
  std::string id = std::get<std::string>(node->children[0]->v);
  Type type; 
  if (node->children[1]->node == Node::TYPE) {
    Type type_left = node->children[1]->type;
    if (node->children.size() > 2) {
      visit_expr(node->children[2].get());
      Type type_right = node->children[2]->type;
      type = type_unify(type_left, type_right);
    } else {
      type = type_left;
    }
  } else {
    visit_expr(node->children[1].get());
    type = node->children[1]->type;
  }
  if (type.is_null) {
    type_error(node);
  }
  node->type = type;
  SymbolInfo sym_info(type, false);
  bool success = st->declare(id, sym_info);
  if (!success) {
    error(node);
    std::cerr << ": variable " << id << " already declared\n";
    exit(1);
  }
}
inline void SemanticAnalyzer::visit_id(AST *const node) {
  SymbolInfo *sym_info = st->lookup(std::get<std::string>(node->v));
  if (!sym_info) {
    error(node);
    std::cerr << ": variable " << std::get<std::string>(node->v) << " is not declared\n";
    exit(1);
  }
}
inline void SemanticAnalyzer::visit_expr(AST *const node) {
  visit_assign_expr(node);
}
inline void SemanticAnalyzer::visit_assign_expr(AST *const node) {
  if (node->node == Node::ASSIGN) {
  visit_range_expr(node->children[0].get());
  visit_assign_expr(node->children[1].get());
  Type type_left = node->children[0]->type;
  Type type_right = node->children[1]->type;
  Type type = type_unify(type_left, type_right);
  if (type.is_null) {
    type_error(node);
  }
  node->type = type;
  } else {
    visit_range_expr(node);
  }
}
inline void SemanticAnalyzer::visit_range_expr(AST *const node) {
  if (node->node == Node::RANGE) {
  visit_expr(node->children[0].get());
  if (node->children[0]->type != Type::from_type_node(TypeNode::INT)) {
    type_error(node);
  }
  visit_expr(node->children[1].get());
  if (node->children[1]->type != Type::from_type_node(TypeNode::INT)) {
    type_error(node);
  }
  if (node->children.size() > 2) {
    visit_expr(node->children[2].get());
    if (node->children[2]->type != Type::from_type_node(TypeNode::INT)) {
      type_error(node);
    }
  }
  } else {
    visit_logical_or_expr(node);
  }
}
inline void SemanticAnalyzer::visit_logical_or_expr(AST *const node) {
  if (auto p = std::get_if<LogicalOpNode>(&node->v); p && *p == LogicalOpNode::OR) {
  visit_logical_or_expr(node->children[0].get());
  visit_logical_or_expr(node->children[1].get());
  Type type_left = type_unify(Type::from_type_node(TypeNode::BOOL), node->children[0]->type);
  if (type_left.is_null) {
    type_error(node);
  }
  Type type_right = type_unify(Type::from_type_node(TypeNode::BOOL), node->children[1]->type);
  if (type_right.is_null) {
    type_error(node);
  }
  node->type = Type::from_type_node(TypeNode::BOOL);
  } else {
    visit_logical_and_expr(node);
  }
}
inline void SemanticAnalyzer::visit_logical_and_expr(AST *const node) {
  if (auto p = std::get_if<LogicalOpNode>(&node->v); p && *p == LogicalOpNode::AND) {
  visit_logical_and_expr(node->children[0].get());
  visit_logical_and_expr(node->children[1].get());
  Type type_left = type_unify(Type::from_type_node(TypeNode::BOOL), node->children[0]->type);
  if (type_left.is_null) {
    type_error(node);
  }
  Type type_right = type_unify(Type::from_type_node(TypeNode::BOOL), node->children[1]->type);
  if (type_left.is_null) {
    type_error(node);
  }
  node->type = Type::from_type_node(TypeNode::BOOL);
  } else {
    visit_bitwise_or_expr(node);
  }
}
inline void SemanticAnalyzer::visit_bitwise_or_expr(AST *const node) {
  if (auto p = std::get_if<BitwiseOpNode>(&node->v); p && *p == BitwiseOpNode::BITWISE_OR) {
  visit_bitwise_or_expr(node->children[0].get());
  visit_bitwise_or_expr(node->children[1].get());
  Type type_left = type_unify(Type::from_type_node(TypeNode::INT), node->children[0]->type);
  if (type_left.is_null) {
    type_error(node);
  }
  Type type_right = type_unify(Type::from_type_node(TypeNode::INT), node->children[1]->type);
  if (type_right.is_null) {
    type_error(node);
  }
  node->type = Type::from_type_node(TypeNode::INT);
  } else {
    visit_bitwise_xor_expr(node);
  }
}
inline void SemanticAnalyzer::visit_bitwise_xor_expr(AST *const node) {
  if (auto p = std::get_if<BitwiseOpNode>(&node->v); p && *p == BitwiseOpNode::BITWISE_XOR) {
  visit_bitwise_xor_expr(node->children[0].get());
  visit_bitwise_xor_expr(node->children[1].get());
  Type type_left = type_unify(Type::from_type_node(TypeNode::INT), node->children[0]->type);
  if (type_left.is_null) {
    type_error(node);
  }
  Type type_right = type_unify(Type::from_type_node(TypeNode::INT), node->children[1]->type);
  if (type_right.is_null) {
    type_error(node);
  }
  node->type = Type::from_type_node(TypeNode::INT);
  } else {
    visit_bitwise_and_expr(node);
  }
}
inline void SemanticAnalyzer::visit_bitwise_and_expr(AST *const node) {
  if (auto p = std::get_if<BitwiseOpNode>(&node->v); p && *p == BitwiseOpNode::BITWISE_AND) {
  visit_bitwise_and_expr(node->children[0].get());
  visit_bitwise_and_expr(node->children[1].get());
  Type type_left = type_unify(Type::from_type_node(TypeNode::INT), node->children[0]->type);
  if (type_left.is_null) {
    type_error(node);
  }
  Type type_right = type_unify(Type::from_type_node(TypeNode::INT), node->children[1]->type);
  if (type_right.is_null) {
    type_error(node);
  }
  node->type = Type::from_type_node(TypeNode::INT);
  } else {
    visit_equality_expr(node);
  }
}
inline void SemanticAnalyzer::visit_equality_expr(AST *const node) {
  if (std::holds_alternative<EqualityOpNode>(node->v)) {
    visit_equality_expr(node->children[0].get());
    visit_equality_expr(node->children[1].get());
    Type type_left = node->children[0]->type;
    Type type_right = node->children[1]->type;
    if (type_unify(type_left, type_right).is_null) {
      type_error(node);
    }
    node->type = Type::from_type_node(TypeNode::BOOL);
  } else {
    visit_relational_expr(node);
  }
}
inline void SemanticAnalyzer::visit_relational_expr(AST *const node) {
  if (std::holds_alternative<RelationalOpNode>(node->v)) {
    visit_relational_expr(node->children[0].get());
    visit_relational_expr(node->children[1].get());
    Type type_left = node->children[0]->type;
    Type type_right = node->children[1]->type;
    Type type = type_unify(type_left, type_right);
    if (type.is_null) {
      type_error(node);
    }
    switch (type.type_node) {
      case TypeNode::VECTOR:
      case TypeNode::MATRIX:
      case TypeNode::TENSOR:
        type_error(node);
      default:
        break;
    }
    node->type = Type::from_type_node(TypeNode::BOOL);
  } else {
   visit_additive_expr(node);
  }
}
inline void SemanticAnalyzer::visit_additive_expr(AST *const node) {
  if (std::holds_alternative<AdditiveOpNode>(node->v)) {
    visit_additive_expr(node->children[0].get());
    visit_additive_expr(node->children[1].get());
    Type left = node->children[0]->type;
    Type right = node->children[1]->type;
  }
}
inline void SemanticAnalyzer::visit_const_decl(AST *const node) {
  std::string id = std::get<std::string>(node->children[0]->v);
  Type type;
  if (node->children[1]->node == Node::TYPE) {
    Type type_left = node->children[1]->type;
    visit_expr(node->children[2].get());
    Type type_right = node->children[2]->type;
    type = type_unify(type_left, type_right);
  } else {
    visit_expr(node->children[1].get());
    type = node->children[1]->type;
  }
  SymbolInfo sym_info(type, true);
  bool success = st->declare(id, sym_info);
  if (!success) {
    error(node);
    std::cerr << ": const variable " << id << " already defined\n";
    exit(1);
  }
}
inline Type SemanticAnalyzer::type_unify(const Type& type_left, const Type& type_right) {
  switch (type_left.type_node) {
    case TypeNode::INT:
    case TypeNode::FLOAT:
    case TypeNode::BOOL:
      return base_type_node_unify(type_left, type_right);
    case TypeNode::VECTOR:
    case TypeNode::MATRIX:
    case TypeNode::TENSOR:
      return container_type_unify(type_left, type_right);
    default:
      return Type::error();
  }
}
inline Type SemanticAnalyzer::base_type_node_unify(TypeNode left, TypeNode right) {
  switch (left) {
    case TypeNode::INT:
    case TypeNode::FLOAT:
    case TypeNode::BOOL:
      switch (right) {
        case TypeNode::INT:
        case TypeNode::FLOAT:
        case TypeNode::BOOL:
          return Type::from_type_node(left);
        default:
          return Type::error();
      }
    default:
      return Type::error();
  }
}
inline Type SemanticAnalyzer::container_type_unify(const Type& left, const Type& right) {
  switch (right.type_node) {
    case TypeNode::INT:
    case TypeNode::BOOL:
    case TypeNode::FLOAT:
    case TypeNode::VOID:
      return Type::error();
    default:
      if (left.sizes != right.sizes) {
        return Type::error();
      }
      TypeNode base_type = base_type_node_unify(left.base_type, right.base_type).type_node;
      Type t = Type::from_type_node(left.type_node);
      t.base_type = base_type;
      t.sizes = left.sizes;
      return t;
  }
}
inline Type SemanticAnalyzer::super_base_type_unify(TypeNode left, TypeNode right) {
  //TODO
  //WE SELECT THE HIGHER TYPE (THE SUPER TYPE)
  switch (left) {
    case TypeNode::BOOL:
      return right;
    case TypeNode::INT:
      switch (right) {
        case TypeNode::FLOAT:
          return right;
        default:
          return left;
      }
    default:
      return left;
  }
}
