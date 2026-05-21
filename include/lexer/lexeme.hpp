#pragma once
#include "token.hpp"
#include <string>
struct Lexeme {
  int start = -1;
  int length = 0;
  int line_number = -1;
  int col = -1;
  std::string s = "";
  Token tok = Token::TOK_ERROR;
  Lexeme() = default;
  Lexeme(int start, int length, int line_number, int col, std::string s, Token tok)
      : start(start), length(length), line_number(line_number), col(col), s(s), tok(tok) {
  }
};
