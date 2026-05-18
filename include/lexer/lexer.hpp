#pragma once
#include "lexeme.hpp"
#include "regex_to_nfa.hpp"
#include "token.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
inline const std::string numeric_regex() {
  std::string ret;
  ret.push_back('(');
  for (int i = 0; i < 9; i++) {
    ret.push_back((char)('0' + i));
    ret.push_back('+');
  }
  ret.push_back('9');
  ret.push_back(')');
  return ret;
}
inline const std::string numeric_without_zero_regex() {
  std::string ret;
  ret.push_back('(');
  for (int i = 1; i < 9; i++) {
    ret.push_back((char)('0' + i));
    ret.push_back('+');
  }
  ret.push_back('9');
  ret.push_back(')');
  return ret;
}
inline const std::string alpha_regex() {
  std::string ret;
  ret.push_back('(');
  for (int i = 0; i < 26; i++) {
    ret.push_back((char)('a' + i));
    ret.push_back('+');
  }
  for (int i = 0; i < 25; i++) {
    ret.push_back((char)('A' + i));
    ret.push_back('+');
  }
  ret.push_back('Z');
  ret.push_back(')');
  return ret;
}
inline const std::string int_lit_regex() {
  std::string ret;
  ret.append("((0+");
  ret.append(numeric_without_zero_regex());
  ret.append(numeric_regex());
  ret.append("*))");
  return ret;
}
inline const std::string float_lit_regex() {
  std::string ret;
  std::string int_lit = int_lit_regex();
  ret.append("((");
  ret.append(int_lit);
  ret.append("+(\\e+");
  ret.append(int_lit);
  ret.append(")\\.");
  ret.append(int_lit);
  ret.append("+");
  ret.append(int_lit);
  ret.append("\\.(\\e+");
  ret.append(int_lit);
  ret.append("))(\\e+(e+E)(\\e+\\++-)");
  ret.append(int_lit);
  ret.append("))");
  return ret;
}
inline const std::string id_regex() {
  std::string alpha = alpha_regex();
  std::string numeric = numeric_regex();
  std::string ret;
  ret.append("((_+");
  ret.append(alpha);
  ret.append(")(_+");
  ret.append(alpha);
  ret.append("+");
  ret.append(numeric);
  ret.append(")*)");
  return ret;
}
const std::vector<std::string> regexes = {"fn",
                                          "let",
                                          "return",
                                          "print",
                                          "println",
                                          "const",
                                          "tensor",
                                          "matrix",
                                          "vector",
                                          "size",
                                          "int",
                                          "float",
                                          "bool",
                                          "if",
                                          "else",
                                          "range",
                                          "for",
                                          "while",
                                          "in",
                                          "break",
                                          "continue",
                                          "true",
                                          "false",
                                          int_lit_regex(),
                                          float_lit_regex(),
                                          "=",
                                          "\\+",
                                          "-",
                                          "\\*",
                                          "/",
                                          "\\.\\*",
                                          "\\./",
                                          "==",
                                          "!=",
                                          "<",
                                          "<=",
                                          ">",
                                          ">=",
                                          "and",
                                          "&",
                                          "or",
                                          "|",
                                          "not",
                                          "~",
                                          "^",
                                          "'",
                                          "->",
                                          ":",
                                          ",",
                                          ";",
                                          "\\.",
                                          "\\(",
                                          "\\)",
                                          "[",
                                          "]",
                                          "{",
                                          "}",
                                          "#\\?",
                                          id_regex()};
#define X(name) Token::name,
const std::vector<Token> tokens = {TOKEN_LIST};
#undef X
inline const std::vector<Token> tokens_truncated() {
  std::vector<Token> ret;
  for (int i = 0; i < tokens.size() - 3; i++) {
    ret.push_back(tokens[i]);
  }
  return ret;
}
const DFA dfa = regex_to_dfa(regexes, tokens_truncated());
inline std::vector<Lexeme> maximal_munch(std::string &source) {
  std::vector<Lexeme> lexemes;
  std::unordered_set<long long> failed;
  std::stack<std::pair<int, int>> s;
  int pos = 0;
  int line_number = 1;
  int pos_within_line = 1;
  int last_accepting_pos_within_line = 0;
  while (pos < source.size()) {
    int last_accepting_state = -1;
    int last_accepting_pos = -1;
    int state = dfa.start_state;
    int start = pos;
    while (pos < source.size()) {
      long long key = (static_cast<long long>(pos) << 32) | state;
      if (failed.count(key)) {
        break;
      }
      unsigned char x = static_cast<unsigned char>(source[pos]);
      if (x & 0x80) {
        std::cerr << "Lexical error in line " << line_number << " at position "
                  << pos_within_line << "\n";
        std::exit(1);
      }
      state = dfa.delta[state][x];
      if (dfa.token_types[state] != Token::TOK_ERROR) {
        s = std::stack<std::pair<int, int>>();
        last_accepting_state = state;
        last_accepting_pos = pos;
        last_accepting_pos_within_line = pos_within_line;
      }
      s.push(std::make_pair(pos, state));
      pos++;
      pos_within_line++;
    }
    while (!s.empty() && dfa.token_types[s.top().second] == Token::TOK_ERROR) {
      failed.insert((static_cast<long long>(s.top().first) << 32) |
                    s.top().second);
      s.pop();
    }
    if (last_accepting_state == -1) {
      std::cerr << "Lexical error in line " << line_number << " at position "
                << last_accepting_pos_within_line + 1 << "\n";
      std::exit(1);
    }
    Lexeme lexeme(start, last_accepting_pos - start + 1, line_number,
                  source.substr(start, last_accepting_pos - start + 1),
                  dfa.token_types[last_accepting_state]);
    if (lexeme.tok != Token::TOK_COMMENT) {
      lexemes.push_back(lexeme);
    }
    pos = last_accepting_pos + 1;
    pos_within_line = last_accepting_pos_within_line + 1;
    while (pos < source.size() &&
           std::isspace(static_cast<unsigned char>(source[pos]))) {
      if (source[pos] == '\n') {
        line_number++;
        pos_within_line = 1;
      } else {
        pos_within_line++;
      }
      pos++;
    }
  }
  lexemes.push_back(Lexeme(-1, -1, -1, "", Token::TOK_EOF));
  return lexemes;
}
