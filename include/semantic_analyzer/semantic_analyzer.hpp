#pragma once
#include "parser/ast.hpp"
#include "parser/type.hpp"
#include "symbol_table.hpp"
#include <iostream>
struct SemanticAnalyzer {
  std::unique_ptr<SymbolTable> st;
  std::optional<TypeNode> curr_func_ret_type;
  SemanticAnalyzer(AST *const root) : st(std::make_unique<SymbolTable>()) {
    visit_program(root);
  }
  [[noreturn]] void mismatched_dimensions_error(AST *const root) {
    std::cerr << "Mismatched dimensions error in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_range_expr(AST *const root) {
    std::cerr << "Error: Invalid range expression in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_matrix_dimensions_error(AST *const root) {
    std::cerr << "Error: Matrix to be inversed must be square; Line "
              << root->lexeme.line_number << ", Position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_matrix_inverse_argument_error(AST *const root) {
    std::cerr << "Error: Argument to matrix inverse must be a matrix; Line "
              << root->lexeme.line_number << ", Position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void duplicate_parameter_name_error(AST *const root) {
    std::cerr << "Error: Duplicate parameter name in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void function_already_defined_error(AST *const root) {
    std::cerr << "Error: Function in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << " already defined\n";
    exit(1);
  }
  [[noreturn]] void invalid_variable_declaration_error(AST *const root) {
    std::cerr << "Invalid variable declaration in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << ": Variable already declared\n";
    exit(1);
  }
  [[noreturn]] void invalid_if_stmt_predicate_error(AST *const root) {
    std::cerr << "Invalid predicate in if statement in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_while_loop_predicate_error(AST *const root) {
    std::cerr << "Invalid predicate in while loop in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_const_definition_error(AST *const root) {
    std::cerr << "Invalid const definition in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col
              << ": Const variable already defined\n";
    exit(1);
  }
  [[noreturn]] void invalid_print_argument_error(AST *const root) {
    std::cerr << "Invalid print argument error in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void variable_not_declared_error(AST *const root) {
    std::cerr << "Variable not found error in line " << root->lexeme.line_number
              << "at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void error(AST *const root) {
    std::cerr << "Semantic error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void const_assign_error(AST *const root) {
    std::cerr << "Invalid assignment error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col
              << ": Can't asssign to const variable\n";
    exit(1);
  }
  [[noreturn]] void transpose_error(AST *const root) {
    std::cerr << "Invalid transpose error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col
              << ": Object to be transposed must be a matrix\n";
    exit(1);
  }
  [[noreturn]] void type_error(AST *const root) {
    std::cerr << "Type mismatch error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_assignment_error(AST *const root) {
    std::cerr << "Invalid assignment error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col
              << ": Value assigned to must be an l-value\n";
    exit(1);
  }
  [[noreturn]] void index_error(AST *const root) {
    std::cerr << "Index error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_index_error(AST *const root) {
    std::cerr << "Invalid index error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void argument_count_mismatch_error(AST *const root) {
    std::cerr << "Argument count mismatch error in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void argument_type_mismatch_error(AST *const root) {
    std::cerr << "Argument type mismatch error in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_function_call_error(AST *const root) {
    std::cerr << "Invalid function call in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void undeclared_function_call_error(AST *const root) {
    std::cerr << "Undeclared function call in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void function_ret_type_error(AST *const root) {
    std::cerr << "Function return type mismatch error in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void symbol_not_found_error(AST *const root) {
    std::cerr << "Symbol not found in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col << "\n";
    exit(1);
  }
  [[noreturn]] void invalid_tensor_declaration_error(AST *const root) {
    std::cerr << "Invalid tensor declaration in line "
              << root->lexeme.line_number << " at position " << root->lexeme.col
              << "\n";
    exit(1);
  }
  [[noreturn]] void control_paths_error(AST *const root) {
    std::cerr << "Semantic error in line " << root->lexeme.line_number
              << " at position " << root->lexeme.col
              << ": Not all control paths return a value\n";
    exit(1);
  }
  void visit_program(AST *const node);
  void visit_decl(AST *const node);
  void visit_var_decl(AST *const node);
  static Type type_unify(const Type &type_left, const Type &type_right);
  static Type base_type_node_unify(TypeNode left, TypeNode right);
  static Type container_type_unify(const Type &left, const Type &right);
  static Type super_type_unify(const Type &left, const Type &right);
  static Type super_base_type_node_unify(TypeNode left, TypeNode right);
  static Type super_container_type_unify(const Type &left, const Type &right);
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
  bool check_returns(AST *const node);
  void visit_function_decl(AST *const node);
  void visit_param(AST *const node);
  void visit_block(AST *const node);
  void visit_if_stmt(AST *const node);
  void visit_while_stmt(AST *const node);
  void visit_for_stmt(AST *const node);
  void visit_return_stmt(AST *const node);
  void visit_print_stmt(AST *const node);
  void visit_expr_stmt(AST *const node);
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
    invalid_variable_declaration_error(node);
  }
}
inline void SemanticAnalyzer::visit_id(AST *const node) {
  SymbolInfo *sym_info = st->lookup(std::get<std::string>(node->v));
  if (!sym_info) {
    variable_not_declared_error(node);
  }
}
inline void SemanticAnalyzer::visit_expr(AST *const node) {
  visit_assign_expr(node);
}
inline void SemanticAnalyzer::visit_assign_expr(AST *const node) {
  if (node->node == Node::ASSIGN) {
    visit_range_expr(node->children[0].get());
    auto left_node = node->children[0].get();
    bool is_id = left_node->node == Node::ID;
    bool is_index =
        left_node->node == Node::POSTFIX_OP &&
        std::get<PostfixOpNode>(left_node->v) == PostfixOpNode::INDEX;
    if (!is_id && !is_index) {
      invalid_assignment_error(node);
    }
    if (is_id) {
      auto sym_info = st->lookup(std::get<std::string>(node->children[0]->v));
      if (!sym_info) {
        variable_not_declared_error(node);
      }
      if (sym_info->is_const) {
        const_assign_error(node);
      }
    }
    if (is_index) {
      auto curr = node->children[0]->children[0].get();
      if (curr->node != Node::ID) {
        invalid_assignment_error(node);
      }
      std::string base_name = std::get<std::string>(curr->v);
      auto base_sym_info = st->lookup(base_name);
      if (!base_sym_info) {
        variable_not_declared_error(node);
      }
      if (base_sym_info->is_const) {
        const_assign_error(node);
      }
    }
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
    if (node->children[0]->type.type_node != TypeNode::INT) {
      type_error(node);
    }
    visit_expr(node->children[1].get());
    if (node->children[1]->type.type_node != TypeNode::INT) {
      type_error(node);
    }
    if (node->children.size() > 2) {
      visit_expr(node->children[2].get());
      if (node->children[2]->type.type_node != TypeNode::INT) {
        type_error(node);
      }
    }
    node->type = Type::from_type_node(TypeNode::VECTOR);
    node->type.base_type = TypeNode::INT;
    node->type.sizes = {static_cast<int64_t>(node->children.size())};
  } else {
    visit_logical_or_expr(node);
  }
}
inline void SemanticAnalyzer::visit_logical_or_expr(AST *const node) {
  if (auto p = std::get_if<LogicalOpNode>(&node->v);
      p && *p == LogicalOpNode::OR) {
    visit_logical_or_expr(node->children[0].get());
    visit_logical_or_expr(node->children[1].get());
    Type type_left = type_unify(Type::from_type_node(TypeNode::BOOL),
                                node->children[0]->type);
    if (type_left.is_null) {
      type_error(node);
    }
    Type type_right = type_unify(Type::from_type_node(TypeNode::BOOL),
                                 node->children[1]->type);
    if (type_right.is_null) {
      type_error(node);
    }
    node->type = Type::from_type_node(TypeNode::BOOL);
  } else {
    visit_logical_and_expr(node);
  }
}
inline void SemanticAnalyzer::visit_logical_and_expr(AST *const node) {
  if (auto p = std::get_if<LogicalOpNode>(&node->v);
      p && *p == LogicalOpNode::AND) {
    visit_logical_and_expr(node->children[0].get());
    visit_logical_and_expr(node->children[1].get());
    Type type_left = type_unify(Type::from_type_node(TypeNode::BOOL),
                                node->children[0]->type);
    if (type_left.is_null) {
      type_error(node);
    }
    Type type_right = type_unify(Type::from_type_node(TypeNode::BOOL),
                                 node->children[1]->type);
    if (type_right.is_null) {
      type_error(node);
    }
    node->type = Type::from_type_node(TypeNode::BOOL);
  } else {
    visit_bitwise_or_expr(node);
  }
}
inline void SemanticAnalyzer::visit_bitwise_or_expr(AST *const node) {
  if (auto p = std::get_if<BitwiseOpNode>(&node->v);
      p && *p == BitwiseOpNode::BITWISE_OR) {
    visit_bitwise_or_expr(node->children[0].get());
    visit_bitwise_or_expr(node->children[1].get());
    Type type_left = type_unify(Type::from_type_node(TypeNode::INT),
                                node->children[0]->type);
    if (type_left.is_null) {
      type_error(node);
    }
    Type type_right = type_unify(Type::from_type_node(TypeNode::INT),
                                 node->children[1]->type);
    if (type_right.is_null) {
      type_error(node);
    }
    node->type = Type::from_type_node(TypeNode::INT);
  } else {
    visit_bitwise_xor_expr(node);
  }
}
inline void SemanticAnalyzer::visit_bitwise_xor_expr(AST *const node) {
  if (auto p = std::get_if<BitwiseOpNode>(&node->v);
      p && *p == BitwiseOpNode::BITWISE_XOR) {
    visit_bitwise_xor_expr(node->children[0].get());
    visit_bitwise_xor_expr(node->children[1].get());
    Type type_left = type_unify(Type::from_type_node(TypeNode::INT),
                                node->children[0]->type);
    if (type_left.is_null) {
      type_error(node);
    }
    Type type_right = type_unify(Type::from_type_node(TypeNode::INT),
                                 node->children[1]->type);
    if (type_right.is_null) {
      type_error(node);
    }
    node->type = Type::from_type_node(TypeNode::INT);
  } else {
    visit_bitwise_and_expr(node);
  }
}
inline void SemanticAnalyzer::visit_bitwise_and_expr(AST *const node) {
  if (auto p = std::get_if<BitwiseOpNode>(&node->v);
      p && *p == BitwiseOpNode::BITWISE_AND) {
    visit_bitwise_and_expr(node->children[0].get());
    visit_bitwise_and_expr(node->children[1].get());
    Type type_left = type_unify(Type::from_type_node(TypeNode::INT),
                                node->children[0]->type);
    if (type_left.is_null) {
      type_error(node);
    }
    Type type_right = type_unify(Type::from_type_node(TypeNode::INT),
                                 node->children[1]->type);
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
    Type type = super_type_unify(left, right);
    if (type.is_null) {
      type_error(node);
    }
    node->type = type;
  } else {
    visit_multiplicative_expr(node);
  }
}
inline void SemanticAnalyzer::visit_multiplicative_expr(AST *const node) {
  if (std::holds_alternative<MultiplicativeOpNode>(node->v)) {
    visit_multiplicative_expr(node->children[0].get());
    visit_multiplicative_expr(node->children[1].get());
    Type left = node->children[0]->type;
    Type right = node->children[1]->type;
    if (std::get<MultiplicativeOpNode>(node->v) ==
        MultiplicativeOpNode::MAT_MUL) {
      if (left.type_node != TypeNode::MATRIX ||
          right.type_node != TypeNode::MATRIX) {
        type_error(node);
      }
      Type base = super_base_type_node_unify(left.base_type, right.base_type);
      if (base.is_null) {
        type_error(node);
      }
      if (left.sizes[1] != right.sizes[0]) {
        mismatched_dimensions_error(node);
      }
      node->type = Type::from_type_node(TypeNode::MATRIX);
      node->type.base_type = base.type_node;
      node->type.sizes = {left.sizes[0], right.sizes[1]};
    } else {
      Type type = super_type_unify(left, right);
      if (type.is_null) {
        type_error(node);
      }
      node->type = type;
    }
  } else {
    visit_unary_expr(node);
  }
}
inline void SemanticAnalyzer::visit_unary_expr(AST *const node) {
  if (std::holds_alternative<UnaryOpNode>(node->v)) {
    visit_unary_expr(node->children[0].get());
    switch (node->children[0]->type.type_node) {
    case TypeNode::VECTOR:
    case TypeNode::MATRIX:
    case TypeNode::TENSOR:
      type_error(node);
    }
    switch (std::get<UnaryOpNode>(node->v)) {
    case UnaryOpNode::MINUS:
    case UnaryOpNode::PLUS:
      node->type = node->children[0]->type;
      break;
    case UnaryOpNode::NOT: {
      Type type = node->children[0]->type;
      type.type_node = TypeNode::BOOL;
      node->type = type;
      break;
    }
    default:
      if (node->children[0]->type.type_node == TypeNode::FLOAT) {
        type_error(node);
      }
      Type type = node->children[0]->type;
      type.type_node = TypeNode::INT;
      node->type = type;
    }
  } else {
    visit_postfix_expr(node);
  }
}
inline void SemanticAnalyzer::visit_postfix_expr(AST *const node) {
  if (std::holds_alternative<PostfixOpNode>(node->v)) {
    visit_postfix_expr(node->children[0].get());
    switch (std::get<PostfixOpNode>(node->v)) {
    case PostfixOpNode::INDEX:
      switch (node->children[0]->type.type_node) {
      case TypeNode::BOOL:
      case TypeNode::INT:
      case TypeNode::FLOAT:
        type_error(node);
      default:
        if (node->children[0]->type.sizes.size() != node->children.size() - 1) {
          index_error(node);
        }
        for (int i = 1; i < node->children.size(); i++) {
          visit_expr(node->children[i].get());
          if (node->children[i]->type.type_node != TypeNode::INT) {
            invalid_index_error(node->children[i].get());
          }
        }
        node->type = Type::from_type_node(node->children[0]->type.base_type);
      }
      break;
    case PostfixOpNode::ARGUMENT:
      if (std::holds_alternative<std::string>(node->children[0]->v)) {
        std::string name = std::get<std::string>(node->children[0]->v);
        if (!st->lookup(name) || !st->lookup(name)->is_function) {
          undeclared_function_call_error(node);
        }
        if (st->lookup(name)->param_types.size() != node->children.size() - 1) {
          argument_count_mismatch_error(node);
        }
        for (int i = 1; i < node->children.size(); i++) {
          visit_expr(node->children[i].get());
          if (type_unify(st->lookup(name)->param_types[i - 1],
                         node->children[i]->type)
                  .is_null) {
            argument_type_mismatch_error(node);
          }
        }
        node->type = st->lookup(name)->type;
      } else {
        invalid_function_call_error(node);
      }
      break;
    case PostfixOpNode::DOT:
      switch (node->children[0]->type.type_node) {
      case TypeNode::BOOL:
      case TypeNode::INT:
      case TypeNode::FLOAT:
        type_error(node);
      default:
        node->type = Type::from_type_node(TypeNode::VECTOR);
        node->type.base_type = TypeNode::INT;
        node->type.sizes = std::vector<int64_t>{
            static_cast<int64_t>(node->children[0]->type.sizes.size())};
      }
      break;
    case PostfixOpNode::TRANSPOSE:
      switch (node->children[0]->type.type_node) {
      case TypeNode::MATRIX:
        node->type = node->children[0]->type;
        node->type.sizes = std::vector<int64_t>{
            node->children[0]->type.sizes[1], node->children[0]->type.sizes[0]};
        break;
      default:
        transpose_error(node);
      }
      break;
    default:
      switch (node->children[0]->type.type_node) {
      case TypeNode::MATRIX:
        if (node->children[0]->type.sizes[0] !=
            node->children[0]->type.sizes[1]) {
          invalid_matrix_dimensions_error(node);
        }
        node->type = node->children[0]->type;
        break;
      default:
        invalid_matrix_inverse_argument_error(node);
      }
    }
  } else {
    visit_primary_expr(node);
  }
}
inline void SemanticAnalyzer::visit_primary_expr(AST *const node) {
  switch (node->node) {
  case Node::INT_LIT:
    node->type = Type::from_type_node(TypeNode::INT);
    break;
  case Node::FLOAT_LIT:
    node->type = Type::from_type_node(TypeNode::FLOAT);
    break;
  case Node::BOOL:
    node->type = Type::from_type_node(TypeNode::BOOL);
    break;
  case Node::STRING_LIT:
    node->type = Type::from_type_node(TypeNode::STRING);
    break;
  case Node::ID: {
    auto ptr = st->lookup(std::get<std::string>(node->v));
    if (ptr) {
      node->type = ptr->type;
    } else {
      symbol_not_found_error(node);
    }
    break;
  }
  case Node::BINARY_OP:
  case Node::UNARY_OP:
  case Node::POSTFIX_OP:
  case Node::FN_CALL:
  case Node::RANGE:
  case Node::ASSIGN:
    visit_expr(node);
    break;
  case Node::TENSOR_INIT: {
    Type target_type = node->children[0]->type;
    visit_expr(node->children[1].get());
    Type default_val_type = node->children[1]->type;
    Type base_as_type = Type::from_type_node(target_type.base_type);
    Type unified = type_unify(base_as_type, default_val_type);
    if (unified.is_null) {
      type_error(node);
    }
    node->type = target_type;
    break;
  }
  default:
    if (node->children.size() == 0) {
      node->type = Type::from_type_node(TypeNode::VECTOR);
      node->type.base_type = TypeNode::INT;
      node->type.sizes = {0};
      return;
    }
    visit_expr(node->children[0].get());
    Type type = node->children[0]->type;
    if (type.type_node == TypeNode::VECTOR && type.sizes[0] == 0) {
      invalid_tensor_declaration_error(node);
    }
    for (int i = 1; i < node->children.size(); i++) {
      visit_expr(node->children[i].get());
      if (node->children[i]->type.type_node == TypeNode::VECTOR &&
          node->children[i]->type.sizes[0] == 0) {
        invalid_tensor_declaration_error(node);
      }
      type = super_type_unify(type, node->children[i]->type);
      if (type.is_null) {
        type_error(node);
      }
    }
    Type new_type;
    switch (type.type_node) {
    case TypeNode::BOOL:
    case TypeNode::INT:
    case TypeNode::FLOAT:
      new_type = Type::from_type_node(TypeNode::VECTOR);
      new_type.base_type = type.type_node;
      new_type.sizes =
          std::vector<int64_t>{static_cast<int64_t>(node->children.size())};
      node->type = new_type;
      break;
    default:
      new_type = Type::from_type_node(TypeNode::TENSOR);
      if (type.type_node == TypeNode::VECTOR) {
        new_type = Type::from_type_node(TypeNode::MATRIX);
      }
      new_type.base_type = type.base_type;
      std::vector<int64_t> new_sizes{
          static_cast<int64_t>(node->children.size())};
      for (auto size : type.sizes) {
        new_sizes.push_back(size);
      }
      new_type.sizes = new_sizes;
      node->type = new_type;
    }
    break;
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
  if (type.is_null || type.type_node == TypeNode::VOID) {
    type_error(node);
  }
  type.is_const = true;
  node->type = type;
  SymbolInfo sym_info(type, true);
  bool success = st->declare(id, sym_info);
  if (!success) {
    invalid_const_definition_error(node);
  }
}
inline bool SemanticAnalyzer::check_returns(AST *const node) {
  if (!node) {
    return false;
  }
  switch (node->node) {
  case Node::RETURN_STMT:
    return true;
  case Node::BLOCK:
    for (const auto &child : node->children) {
      if (check_returns(child.get())) {
        return true;
      }
    }
    return false;
  case Node::IF_STMT:
    if (node->children.size() == 3) {
      return check_returns(node->children[1].get()) &&
             check_returns(node->children[2].get());
    }
    return false;
  }
  return false;
}
inline void SemanticAnalyzer::visit_param(AST *const node) {
  node->type = node->children[1]->type;
}
inline void SemanticAnalyzer::visit_function_decl(AST *const node) {
  std::string fn_name = std::get<std::string>(node->children[0]->v);
  std::vector<Type> param_types;
  Type ret_type = Type::from_type_node(TypeNode::VOID);
  for (int i = 1; i < node->children.size() - 1; i++) {
    if (node->children[i]->node == Node::PARAM) {
      param_types.push_back(node->children[i]->children[1]->type);
    } else {
      ret_type = node->children[i]->type;
    }
  }
  SymbolInfo sym_info(ret_type, param_types);
  bool success = st->declare(fn_name, sym_info);
  if (!success) {
    function_already_defined_error(node);
  }
  st->enter_scope();
  for (int i = 1; i < node->children.size() - 1; i++) {
    if (node->children[i]->node == Node::PARAM) {
      SymbolInfo param_sym_info(node->children[i]->children[1]->type,
                                node->children[i]->children[1]->type.is_const);
      bool success =
          st->declare(std::get<std::string>(node->children[i]->children[0]->v),
                      param_sym_info);
      if (!success) {
        duplicate_parameter_name_error(node->children[i].get());
      }
    }
  }
  visit_block(node->children[node->children.size() - 1].get());
  Type actual_ret_type = node->children[node->children.size() - 1]->ret_type;
  Type t = type_unify(ret_type, actual_ret_type);
  if (t.is_null) {
    function_ret_type_error(node);
  }
  st->exit_scope();
}
inline void SemanticAnalyzer::visit_block(AST *const node) {
  if (node->node == Node::RETURN_STMT) {
    visit_return_stmt(node);
    return;
  }
  bool definitely_returns = false;
  Type ret_type = Type::from_type_node(TypeNode::VOID);
  for (const auto &child : node->children) {
    switch (child->node) {
    case Node::CONST_DECL:
      visit_const_decl(child.get());
      break;
    case Node::VAR_DECL:
      visit_var_decl(child.get());
      break;
    case Node::IF_STMT:
      visit_if_stmt(child.get());
      break;
    case Node::WHILE_STMT:
      visit_while_stmt(child.get());
      break;
    case Node::FOR_STMT:
      visit_for_stmt(child.get());
      break;
    case Node::RETURN_STMT:
      visit_return_stmt(child.get());
      break;
    case Node::PRINT_STMT:
      visit_print_stmt(child.get());
      break;
    case Node::EXPR_STMT:
      visit_expr_stmt(child.get());
      break;
    default:
      st->enter_scope();
      visit_block(child.get());
      st->exit_scope();
    }
    if (child->ret_type.is_null) {
    } else if (ret_type.type_node == TypeNode::VOID) {
      ret_type = child->ret_type;
    } else if (child->ret_type.type_node != TypeNode::VOID) {
      ret_type = super_type_unify(ret_type, child->ret_type);
    }
    if (ret_type.is_null) {
      function_ret_type_error(node);
    }
    if (!definitely_returns && check_returns(child.get())) {
      node->ret_type = ret_type;
      definitely_returns = true;
    }
  }
  if (!definitely_returns) {
    if (ret_type.type_node != TypeNode::VOID) {
      control_paths_error(node);
    }
    node->ret_type = ret_type;
  }
}
inline void SemanticAnalyzer::visit_if_stmt(AST *const node) {
  visit_expr(node->children[0].get());
  Type type =
      type_unify(Type::from_type_node(TypeNode::BOOL), node->children[0]->type);
  if (type.is_null) {
    invalid_if_stmt_predicate_error(node->children[0].get());
  }
  node->children[0]->type = type;
  st->enter_scope();
  visit_block(node->children[1].get());
  st->exit_scope();
  Type ret_type = node->children[1]->ret_type;
  if (node->children.size() == 3) {
    st->enter_scope();
    visit_block(node->children[2].get());
    st->exit_scope();
    Type else_ret_type = node->children[2]->ret_type;
    if (ret_type.type_node == TypeNode::VOID) {
      ret_type = else_ret_type;
    } else if (else_ret_type.type_node != TypeNode::VOID) {
      ret_type = super_type_unify(ret_type, else_ret_type);
    }
    if (ret_type.is_null) {
      function_ret_type_error(node);
    }
  }
  node->ret_type = ret_type;
}
inline void SemanticAnalyzer::visit_while_stmt(AST *const node) {
  visit_expr(node->children[0].get());
  Type type =
      type_unify(Type::from_type_node(TypeNode::BOOL), node->children[0]->type);
  if (type.is_null) {
    invalid_while_loop_predicate_error(node);
  }
  node->children[0]->type = type;
  st->enter_scope();
  visit_block(node->children[1].get());
  st->exit_scope();
  node->ret_type = node->children[1]->type;
}
inline void SemanticAnalyzer::visit_for_stmt(AST *const node) {
  st->enter_scope();
  auto var_name = std::get<std::string>(node->children[0]->v);
  visit_expr(node->children[1].get());
  if (node->children[1]->type.type_node != TypeNode::VECTOR) {
    invalid_range_expr(node->children[1].get());
  }
  auto var_type = Type::from_type_node(node->children[1]->type.base_type);
  SymbolInfo sym_info(var_type, false);
  st->declare(var_name, sym_info);
  visit_block(node->children[2].get());
  st->exit_scope();
  node->ret_type = node->children[2]->type;
}
inline void SemanticAnalyzer::visit_return_stmt(AST *const node) {
  if (node->children.size() == 1) {
    visit_expr(node->children[0].get());
    node->type = node->children[0]->type;
  } else {
    node->type = Type::from_type_node(TypeNode::VOID);
  }
  node->ret_type = node->type;
}
inline void SemanticAnalyzer::visit_print_stmt(AST *const node) {
  if (node->children[0]->node == Node::STRING_LIT) {
    node->children[0]->type = Type::from_type_node(TypeNode::STRING);
    return;
  }
  visit_expr(node->children[0].get());
  if (node->children[0]->type.type_node == TypeNode::VOID) {
    invalid_print_argument_error(node);
  }
}
inline void SemanticAnalyzer::visit_expr_stmt(AST *const node) {
  visit_expr(node->children[0].get());
}
inline Type SemanticAnalyzer::type_unify(const Type &type_left,
                                         const Type &type_right) {
  switch (type_left.type_node) {
  case TypeNode::INT:
  case TypeNode::FLOAT:
  case TypeNode::BOOL:
  case TypeNode::VOID:
    return base_type_node_unify(type_left.type_node, type_right.type_node);
  case TypeNode::VECTOR:
  case TypeNode::MATRIX:
  case TypeNode::TENSOR:
    return container_type_unify(type_left, type_right);
  default:
    return Type::error();
  }
}
inline Type SemanticAnalyzer::base_type_node_unify(TypeNode left,
                                                   TypeNode right) {
  if (left == TypeNode::VOID && right == TypeNode::VOID) {
    return Type::from_type_node(TypeNode::VOID);
  }
  if (left == TypeNode::VOID || right == TypeNode::VOID) {
    return Type::error();
  }
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
inline Type SemanticAnalyzer::container_type_unify(const Type &left,
                                                   const Type &right) {
  switch (right.type_node) {
  case TypeNode::INT:
  case TypeNode::BOOL:
  case TypeNode::FLOAT:
  case TypeNode::VOID:
    return Type::error();
  default:
    if (left.sizes != right.sizes || left.is_const != right.is_const) {
      return Type::error();
    }
    Type base_type = base_type_node_unify(left.base_type, right.base_type);
    if (base_type.is_null) {
      return Type::error();
    }
    Type t = Type::from_type_node(left.type_node);
    t.base_type = base_type.type_node;
    t.sizes = left.sizes;
    t.is_const = left.is_const;
    return t;
  }
}
inline Type SemanticAnalyzer::super_base_type_node_unify(TypeNode left,
                                                         TypeNode right) {
  if (left == TypeNode::VOID && right == TypeNode::VOID) {
    return Type::from_type_node(TypeNode::VOID);
  }
  if (left == TypeNode::VOID || right == TypeNode::VOID) {
    return Type::error();
  }
  switch (left) {
  case TypeNode::BOOL:
    return Type::from_type_node(right);
  case TypeNode::INT:
    switch (right) {
    case TypeNode::FLOAT:
      return Type::from_type_node(right);
    default:
      return Type::from_type_node(left);
    }
  case TypeNode::FLOAT:
    return Type::from_type_node(left);
  default:
    return Type::error();
  }
}
inline Type
SemanticAnalyzer::super_container_type_unify(const Type &type_left,
                                             const Type &type_right) {
  switch (type_right.type_node) {
  case TypeNode::BOOL:
  case TypeNode::INT:
  case TypeNode::FLOAT:
  case TypeNode::VOID:
    return Type::error();
  }
  switch (type_left.type_node) {
  case TypeNode::BOOL:
  case TypeNode::INT:
  case TypeNode::FLOAT:
  case TypeNode::VOID:
    return Type::error();
  }
  if (type_left.sizes != type_right.sizes || type_left.is_const != type_right.is_const) {
    return Type::error();
  }
  Type base_type =
      super_base_type_node_unify(type_left.base_type, type_right.base_type);
  if (base_type.is_null) {
    return Type::error();
  }
  Type t;
  if (type_left.type_node == TypeNode::TENSOR ||
      type_right.type_node == TypeNode::TENSOR) {
    t = Type::from_type_node(TypeNode::TENSOR);
  } else {
    t = Type::from_type_node(type_left.type_node);
  }
  t.base_type = base_type.type_node;
  t.sizes = type_left.sizes;
  t.is_const = type_left.is_const;
  return t;
}
inline Type SemanticAnalyzer::super_type_unify(const Type &left,
                                               const Type &right) {
  switch (left.type_node) {
  case TypeNode::BOOL:
  case TypeNode::INT:
  case TypeNode::FLOAT:
  case TypeNode::VOID:
    switch (right.type_node) {
    case TypeNode::BOOL:
    case TypeNode::INT:
    case TypeNode::FLOAT:
    case TypeNode::VOID:
      return super_base_type_node_unify(left.type_node, right.type_node);
    default:
      return Type::error();
    }
  default:
    return super_container_type_unify(left, right);
  }
}
