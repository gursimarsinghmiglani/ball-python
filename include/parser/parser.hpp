#pragma once
#include "ast.hpp"
#include <iostream>
struct Parser {
  const std::vector<Lexeme> &lexemes;
  size_t pos;
  Parser(const std::vector<Lexeme> &lexemes) : lexemes(lexemes), pos(0) {}
  const Lexeme &peek() const { return lexemes[pos]; }
  void advance() { pos++; }
  Token tok() const { return peek().tok; }
  [[noreturn]] void error_function() const {
    std::cerr << "Parsing error in line " << peek().line_number
              << " at position " << pos << "\n";
    std::exit(1);
  }
  bool check(Token t) const { return tok() == t; }
  bool match(Token t) {
    if (tok() == t) {
      advance();
      return true;
    }
    return false;
  }
  void consume(Token t) {
    if (tok() == t) {
      advance();
      return;
    }
    error_function();
  }
  bool is_eof() const { return tok() == Token::TOK_EOF; }
  std::unique_ptr<AST> parse_program();
  std::unique_ptr<AST> parse_id();
  std::unique_ptr<AST> parse_type();
  std::unique_ptr<AST> parse_base_type();
  std::unique_ptr<AST> parse_int_lit();
  std::unique_ptr<AST> parse_expr();
  std::unique_ptr<AST> parse_assign_expr();
  std::unique_ptr<AST> parse_range_expr();
  std::unique_ptr<AST> parse_logical_or_expr();
  std::unique_ptr<AST> parse_logical_and_expr();
  std::unique_ptr<AST> parse_bitwise_or_expr();
  std::unique_ptr<AST> parse_bitwise_xor_expr();
  std::unique_ptr<AST> parse_bitwise_and_expr();
  std::unique_ptr<AST> parse_equality_expr();
  std::unique_ptr<AST> parse_relational_expr();
  std::unique_ptr<AST> parse_additive_expr();
  std::unique_ptr<AST> parse_multiplicative_expr();
  std::unique_ptr<AST> parse_unary_expr();
  std::unique_ptr<AST> parse_postfix_expr();
  void fill_expr_list(std::unique_ptr<AST> &ast);
  std::unique_ptr<AST> parse_primary_expr();
  std::unique_ptr<AST> parse_const_decl();
  std::unique_ptr<AST> parse_function_decl();
  void fill_param_list(std::unique_ptr<AST> &ast);
  std::unique_ptr<AST> parse_param();
  std::unique_ptr<AST> parse_block();
  std::unique_ptr<AST> parse_var_decl();
  std::unique_ptr<AST> parse_if_stmt();
  std::unique_ptr<AST> parse_while_stmt();
  std::unique_ptr<AST> parse_for_stmt();
  std::unique_ptr<AST> parse_return_stmt();
  std::unique_ptr<AST> parse_print_stmt();
  std::unique_ptr<AST> parse_expr_stmt();
  std::unique_ptr<AST> parse_extern_fn_decl();
};
inline std::unique_ptr<AST> Parser::parse_program() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  ast->node = Node::PROGRAM;
  while (!is_eof()) {
    if (check(Token::TOK_LET)) {
      ast->children.push_back(parse_var_decl());
    } else if (check(Token::TOK_CONST)) {
      ast->children.push_back(parse_const_decl());
    } else if (check(Token::TOK_FN)) {
      ast->children.push_back(parse_function_decl());
    } else if (check(Token::TOK_EXTERN)) {
      ast->children.push_back(parse_extern_fn_decl());
    } else {
      error_function();
    }
  }
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_id() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  consume(Token::TOK_ID);
  ast->node = Node::ID;
  ast->v = ast->lexeme.s;
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_type() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  ast->node = Node::TYPE;
  if (match(Token::TOK_VECTOR)) {
    ast->v = TypeNode::VECTOR;
    consume(Token::TOK_LESS);
    auto base_type = parse_base_type()->type;
    consume(Token::TOK_COMMA);
    std::vector<int64_t> sizes;
    sizes.push_back(std::get<int64_t>(parse_int_lit()->v));
    auto t = Type::from_type_node(std::get<TypeNode>(ast->v));
    t.base_type = base_type.type_node;
    t.sizes = sizes;
    ast->type = t;
    consume(Token::TOK_GREATER);
  } else if (match(Token::TOK_MATRIX)) {
    ast->v = TypeNode::MATRIX;
    consume(Token::TOK_LESS);
    auto base_type = parse_base_type()->type;
    consume(Token::TOK_COMMA);
    std::vector<int64_t> sizes;
    sizes.push_back(std::get<int64_t>(parse_int_lit()->v));
    consume(Token::TOK_COMMA);
    sizes.push_back(std::get<int64_t>(parse_int_lit()->v));
    auto t = Type::from_type_node(std::get<TypeNode>(ast->v));
    t.base_type = base_type.type_node;
    t.sizes = sizes;
    ast->type = t;
    consume(Token::TOK_GREATER);
  } else if (match(Token::TOK_TENSOR)) {
    ast->v = TypeNode::TENSOR;
    consume(Token::TOK_LESS);
    auto base_type = parse_base_type()->type;
    consume(Token::TOK_COMMA);
    std::vector<int64_t> sizes;
    sizes.push_back(std::get<int64_t>(parse_int_lit()->v));
    while (match(Token::TOK_COMMA)) {
      sizes.push_back(std::get<int64_t>(parse_int_lit()->v));
    }
    auto t = Type::from_type_node(std::get<TypeNode>(ast->v));
    t.base_type = base_type.type_node;
    t.sizes = sizes;
    ast->type = t;
    consume(Token::TOK_GREATER);
  } else {
    ast = parse_base_type();
  }
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_base_type() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  ast->node = Node::TYPE;
  if (match(Token::TOK_INT)) {
    ast->v = TypeNode::INT;
    ast->type = Type::from_type_node(std::get<TypeNode>(ast->v));
  } else if (match(Token::TOK_FLOAT)) {
    ast->v = TypeNode::FLOAT;
    ast->type = Type::from_type_node(std::get<TypeNode>(ast->v));
  } else if (match(Token::TOK_BOOL)) {
    ast->v = TypeNode::BOOL;
    ast->type = Type::from_type_node(std::get<TypeNode>(ast->v));
  } else {
    error_function();
  }
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_int_lit() {
  if (!check(Token::TOK_INT_LIT)) {
    error_function();
  }
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  ast->node = Node::INT_LIT;
  ast->v = std::stoll(peek().s);
  advance();
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_expr() { return parse_assign_expr(); }
inline std::unique_ptr<AST> Parser::parse_assign_expr() {
  auto left = parse_range_expr();
  if (check(Token::TOK_ASSIGN)) {
    auto lex = peek();
    advance();
    auto right = parse_assign_expr();
    auto ast = std::make_unique<AST>();
    ast->node = Node::ASSIGN;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    ast->lexeme = lex;
    return ast;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_range_expr() {
  if (check(Token::TOK_RANGE)) {
    auto lex = peek();
    advance();
    auto ast = std::make_unique<AST>();
    ast->node = Node::RANGE;
    consume(Token::TOK_LPAREN);
    ast->children.push_back(parse_expr());
    consume(Token::TOK_COMMA);
    ast->children.push_back(parse_expr());
    if (match(Token::TOK_COMMA)) {
      ast->children.push_back(parse_expr());
    }
    consume(Token::TOK_RPAREN);
    ast->lexeme = lex;
    return ast;
  }
  return parse_logical_or_expr();
}
inline std::unique_ptr<AST> Parser::parse_logical_or_expr() {
  auto left = parse_logical_and_expr();
  while (check(Token::TOK_OR)) {
    auto lex = peek();
    advance();
    auto right = parse_logical_and_expr();
    auto ast = std::make_unique<AST>();
    ast->node = Node::BINARY_OP;
    ast->v = LogicalOpNode::OR;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
    left->lexeme = lex;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_logical_and_expr() {
  auto left = parse_bitwise_or_expr();
  while (check(Token::TOK_AND)) {
    auto lex = peek();
    advance();
    auto right = parse_bitwise_or_expr();
    auto ast = std::make_unique<AST>();
    ast->node = Node::BINARY_OP;
    ast->v = LogicalOpNode::AND;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
    left->lexeme = lex;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_bitwise_or_expr() {
  auto left = parse_bitwise_xor_expr();
  while (check(Token::TOK_BITWISE_OR)) {
    auto lex = peek();
    advance();
    auto right = parse_bitwise_xor_expr();
    auto ast = std::make_unique<AST>();
    ast->node = Node::BINARY_OP;
    ast->v = BitwiseOpNode::BITWISE_OR;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
    left->lexeme = lex;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_bitwise_xor_expr() {
  auto left = parse_bitwise_and_expr();
  while (check(Token::TOK_BITWISE_XOR)) {
    auto lex = peek();
    advance();
    auto right = parse_bitwise_and_expr();
    auto ast = std::make_unique<AST>();
    ast->node = Node::BINARY_OP;
    ast->v = BitwiseOpNode::BITWISE_XOR;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
    left->lexeme = lex;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_bitwise_and_expr() {
  auto left = parse_equality_expr();
  while (check(Token::TOK_BITWISE_AND)) {
    auto lex = peek();
    advance();
    auto right = parse_equality_expr();
    auto ast = std::make_unique<AST>();
    ast->node = Node::BINARY_OP;
    ast->v = BitwiseOpNode::BITWISE_AND;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
    left->lexeme = lex;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_equality_expr() {
  auto left = parse_relational_expr();
  while (check(Token::TOK_EQ) || check(Token::TOK_NEQ)) {
    auto lex = peek();
    advance();
    auto ast = std::make_unique<AST>();
    if (match(Token::TOK_EQ)) {
      ast->v = EqualityOpNode::EQ;
    } else {
      ast->v = EqualityOpNode::NEQ;
    }
    auto right = parse_relational_expr();
    ast->node = Node::BINARY_OP;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
    left->lexeme = lex;
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_relational_expr() {
  auto left = parse_additive_expr();
  while (1) {
    auto ast = std::make_unique<AST>();
    if (check(Token::TOK_LESS)) {
      ast->v = RelationalOpNode::LESS;
      ast->lexeme = peek();
      advance();
    } else if (check(Token::TOK_LEQ)) {
      ast->v = RelationalOpNode::LEQ;
      ast->lexeme = peek();
      advance();
    } else if (check(Token::TOK_GREATER)) {
      ast->v = RelationalOpNode::GREATER;
      ast->lexeme = peek();
      advance();
    } else if (check(Token::TOK_GEQ)) {
      ast->v = RelationalOpNode::GEQ;
      ast->lexeme = peek();
      advance();
    } else {
      break;
    }
    auto right = parse_additive_expr();
    ast->node = Node::BINARY_OP;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_additive_expr() {
  auto left = parse_multiplicative_expr();
  while (check(Token::TOK_PLUS) || check(Token::TOK_MINUS)) {
    auto ast = std::make_unique<AST>();
    ast->lexeme = peek();
    if (match(Token::TOK_PLUS)) {
      ast->v = AdditiveOpNode::PLUS;
    } else {
      ast->v = AdditiveOpNode::MINUS;
    }
    auto right = parse_multiplicative_expr();
    ast->node = Node::BINARY_OP;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_multiplicative_expr() {
  auto left = parse_unary_expr();
  while (check(Token::TOK_MUL) || check(Token::TOK_DIV) ||
         check(Token::TOK_DOT_MUL) || check(Token::TOK_DOT_DIV)) {
    auto ast = std::make_unique<AST>();
    ast->lexeme = peek();
    if (match(Token::TOK_MUL)) {
      ast->v = MultiplicativeOpNode::MUL;
    } else if (match(Token::TOK_DIV)) {
      ast->v = MultiplicativeOpNode::DIV;
    } else if (match(Token::TOK_DOT_MUL)) {
      ast->v = MultiplicativeOpNode::DOT_MUL;
    } else {
      ast->v = MultiplicativeOpNode::DOT_DIV;
    }
    auto right = parse_unary_expr();
    ast->node = Node::BINARY_OP;
    ast->children.push_back(std::move(left));
    ast->children.push_back(std::move(right));
    left = std::move(ast);
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_unary_expr() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  if (match(Token::TOK_PLUS)) {
    ast->node = Node::UNARY_OP;
    ast->v = UnaryOpNode::PLUS;
    ast->children.push_back(parse_unary_expr());
    return ast;
  }
  if (match(Token::TOK_MINUS)) {
    ast->node = Node::UNARY_OP;
    ast->v = UnaryOpNode::MINUS;
    ast->children.push_back(parse_unary_expr());
    return ast;
  }
  if (match(Token::TOK_NOT)) {
    ast->node = Node::UNARY_OP;
    ast->v = UnaryOpNode::NOT;
    ast->children.push_back(parse_unary_expr());
    return ast;
  }
  if (match(Token::TOK_BITWISE_NOT)) {
    ast->node = Node::UNARY_OP;
    ast->v = UnaryOpNode::BITWISE_NOT;
    ast->children.push_back(parse_unary_expr());
    return ast;
  }
  return parse_postfix_expr();
}
inline std::unique_ptr<AST> Parser::parse_postfix_expr() {
  auto left = parse_primary_expr();
  while (1) {
    auto ast = std::make_unique<AST>();
    ast->lexeme = peek();
    auto right = std::make_unique<AST>();
    if (match(Token::TOK_LBRACKET)) {
      ast->v = PostfixOpNode::INDEX;
      fill_expr_list(right);
      consume(Token::TOK_RBRACKET);
    } else if (match(Token::TOK_LPAREN)) {
      ast->v = PostfixOpNode::ARGUMENT;
      if (!match(Token::TOK_RPAREN)) {
        fill_expr_list(right);
        consume(Token::TOK_RPAREN);
      }
    } else if (match(Token::TOK_DOT)) {
      ast->v = PostfixOpNode::DOT;
      right = parse_id();
    } else if (match(Token::TOK_TRANSPOSE)) {
      ast->v = PostfixOpNode::TRANSPOSE;
    } else
      break;
    ast->node = Node::POSTFIX_OP;
    ast->children.push_back(std::move(left));
    if (right)
      ast->children.push_back(std::move(right));
    left = std::move(ast);
  }
  return left;
}
inline std::unique_ptr<AST> Parser::parse_primary_expr() {
  auto ast = std::make_unique<AST>();
  if (check(Token::TOK_INT_LIT)) {
    ast->node = Node::INT_LIT;
    ast->v = std::stoll(peek().s);
    ast->lexeme = peek();
    advance();
  } else if (check(Token::TOK_FLOAT_LIT)) {
    ast->node = Node::FLOAT_LIT;
    ast->v = std::stod(peek().s);
    ast->lexeme = peek();
    advance();
  } else if (check(Token::TOK_TRUE)) {
    ast->node = Node::BOOL;
    ast->v = true;
    ast->lexeme = peek();
    advance();
  } else if (check(Token::TOK_FALSE)) {
    ast->node = Node::BOOL;
    ast->v = false;
    ast->lexeme = peek();
    advance();
  } else if (check(Token::TOK_ID)) {
    ast->node = Node::ID;
    ast->v = peek().s;
    ast->lexeme = peek();
    advance();
  } else if (match(Token::TOK_LPAREN)) {
    ast = parse_expr();
    consume(Token::TOK_RPAREN);
  } else if (match(Token::TOK_LBRACKET)) {
    ast->node = Node::TENSOR_LIT;
    if (!match(Token::TOK_RBRACKET)) {
      fill_expr_list(ast);
      consume(Token::TOK_RBRACKET);
    }
  } else {
    error_function();
  }
  return ast;
}
inline void Parser::fill_expr_list(std::unique_ptr<AST> &ast) {
  ast->children.push_back(parse_expr());
  while (match(Token::TOK_COMMA)) {
    ast->children.push_back(parse_expr());
  }
}
inline std::unique_ptr<AST> Parser::parse_const_decl() {
  if (!check(Token::TOK_CONST)) error_function();
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  advance();
  ast->node = Node::CONST_DECL;
  ast->children.push_back(parse_id());
  if (match(Token::TOK_COLON)) {
    ast->children.push_back(parse_type());
  }
  consume(Token::TOK_ASSIGN);
  ast->children.push_back(parse_expr());
  consume(Token::TOK_SEMI);
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_function_decl() {
  if (!check(Token::TOK_FN)) error_function();
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  advance();
  ast->node = Node::FUNCTION_DECL;
  ast->children.push_back(parse_id());
  consume(Token::TOK_LPAREN);
  if (!match(Token::TOK_RPAREN)) {
    fill_param_list(ast);
    consume(Token::TOK_RPAREN);
  }
  if (match(Token::TOK_ARROW)) {
    ast->children.push_back(parse_type());
  }
  ast->children.push_back(parse_block());
  return ast;
}
inline void Parser::fill_param_list(std::unique_ptr<AST> &ast) {
  ast->children.push_back(parse_param());
  while (match(Token::TOK_COMMA)) {
    ast->children.push_back(parse_param());
  }
}
inline std::unique_ptr<AST> Parser::parse_param() {
  auto ast = std::make_unique<AST>();
  ast->node = Node::PARAM;
  ast->children.push_back(parse_id());
  consume(Token::TOK_COLON);
  ast->children.push_back(parse_type());
  ast->lexeme = ast->children[0]->lexeme;
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_block() {
  auto ast = std::make_unique<AST>();
  ast->node = Node::BLOCK;
  ast->lexeme = peek();
  consume(Token::TOK_LBRACE);
  while (!match(Token::TOK_RBRACE)) {
    if (check(Token::TOK_LET)) {
      ast->children.push_back(parse_var_decl());
    } else if (check(Token::TOK_CONST)) {
      ast->children.push_back(parse_const_decl());
    } else if (check(Token::TOK_IF)) {
      ast->children.push_back(parse_if_stmt());
    } else if (check(Token::TOK_WHILE)) {
      ast->children.push_back(parse_while_stmt());
    } else if (check(Token::TOK_FOR)) {
      ast->children.push_back(parse_for_stmt());
    } else if (check(Token::TOK_RETURN)) {
      ast->children.push_back(parse_return_stmt());
    } else if (check(Token::TOK_PRINT)) {
      ast->children.push_back(parse_print_stmt());
    } else if (check(Token::TOK_LBRACE)) {
      ast->children.push_back(parse_block());
    } else {
      ast->children.push_back(parse_expr_stmt());
    }
  }
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_var_decl() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  consume(Token::TOK_LET);
  ast->node = Node::VAR_DECL;
  ast->children.push_back(parse_id());
  if (match(Token::TOK_COLON)) {
    ast->children.push_back(parse_type());
    if (match(Token::TOK_ASSIGN)) {
      ast->children.push_back(parse_expr());
    }
  } else {
    consume(Token::TOK_ASSIGN);
    ast->children.push_back(parse_expr());
  }
  consume(Token::TOK_SEMI);
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_if_stmt() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  consume(Token::TOK_IF);
  ast->node = Node::IF_STMT;
  ast->children.push_back(parse_expr());
  ast->children.push_back(parse_block());
  if (match(Token::TOK_ELSE)) {
    ast->children.push_back(parse_block());
  }
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_while_stmt() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  consume(Token::TOK_WHILE);
  ast->node = Node::WHILE_STMT;
  ast->children.push_back(parse_expr());
  ast->children.push_back(parse_block());
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_for_stmt() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  consume(Token::TOK_FOR);
  ast->node = Node::FOR_STMT;
  ast->children.push_back(parse_id());
  consume(Token::TOK_IN);
  ast->children.push_back(parse_expr());
  ast->children.push_back(parse_block());
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_return_stmt() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  consume(Token::TOK_RETURN);
  ast->node = Node::RETURN_STMT;
  if (!match(Token::TOK_SEMI)) {
    ast->children.push_back(parse_expr());
    consume(Token::TOK_SEMI);
  }
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_print_stmt() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  ast->node = Node::PRINT_STMT;
  if (match(Token::TOK_PRINT)) {
    ast->v = PrintNode::PRINT;
  } else {
    consume(Token::TOK_PRINTLN);
    ast->v = PrintNode::PRINTLN;
  }
  ast->children.push_back(parse_expr());
  consume(Token::TOK_SEMI);
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_expr_stmt() {
  auto ast = std::make_unique<AST>();
  ast->node = Node::EXPR_STMT;
  ast->children.push_back(parse_expr());
  ast->lexeme = ast->children[0]->lexeme;
  consume(Token::TOK_SEMI);
  return ast;
}
inline std::unique_ptr<AST> Parser::parse_extern_fn_decl() {
  auto ast = std::make_unique<AST>();
  ast->lexeme = peek();
  ast->node = Node::EXTERN_DECL;
  consume(Token::TOK_EXTERN);
  consume(Token::TOK_FN);
  ast->children.push_back(parse_id());
  consume(Token::TOK_LPAREN);
  if (!match(Token::TOK_RPAREN)) {
    fill_param_list(ast);
    consume(Token::TOK_RPAREN);
  }
  if (match(Token::TOK_ARROW)) {
    ast->children.push_back(parse_type());
  }
  consume(Token::TOK_SEMI);
  return ast;
}
