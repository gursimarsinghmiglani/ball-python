#pragma once
#include "parser/type_node.hpp"
#include <vector>
struct Type {
  bool is_null = true;
  TypeNode type_node = TypeNode::VOID;
  TypeNode base_type = TypeNode::VOID;
  std::vector<int64_t> sizes;
  bool is_const = false;
  static Type error() {
    Type err;
    err.is_null = true;
    return err;
  }
  static Type from_type_node(TypeNode type_node) {
    Type t;
    t.is_null = false;
    t.type_node = type_node;
    return t;
  }
  bool operator==(const Type &other) const {
    if (is_null != other.is_null || type_node != other.type_node || is_const != other.is_const)
      return false;
    if (type_node == TypeNode::VECTOR || type_node == TypeNode::MATRIX ||
        type_node == TypeNode::TENSOR)
      return base_type == other.base_type && sizes == other.sizes;
    return true;
  }
  bool operator!=(const Type &other) const { return !(*this == other); }
};
