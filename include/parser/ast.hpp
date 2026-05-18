#pragma once
#include "additive_op_node.hpp"
#include "bitwise_op_node.hpp"
#include "equality_op_node.hpp"
#include "lexer/lexeme.hpp"
#include "logical_op_node.hpp"
#include "multiplicative_op_node.hpp"
#include "node.hpp"
#include "postfix_op_node.hpp"
#include "print_node.hpp"
#include "relational_op_node.hpp"
#include "type.hpp"
#include "unary_op_node.hpp"
struct AST {
  Node node;
  std::variant<int64_t, double, bool, std::string, AdditiveOpNode,
               BitwiseOpNode, EqualityOpNode, MultiplicativeOpNode,
               PostfixOpNode, TypeNode, UnaryOpNode, PrintNode,
               RelationalOpNode, LogicalOpNode>
      v;
  std::vector<std::unique_ptr<AST>> children;
  Type type;
  Type ret_type;
  Lexeme lexeme;
};
