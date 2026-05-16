#pragma once
#include "nfa.hpp"
#include <stack>
#include <string>
inline bool is_operator(char x) {
  return x == '*' || x == '.' || x == '+' || x == '(' || x == ')';
}
inline bool concat_valid(const std::string &regex, int i, bool escaped) {
  return (escaped || regex[i] == '*' ||
          regex[i] != '\\' && !is_operator(regex[i]) || regex[i] == ')') &&
         (!is_operator(regex[i + 1]) || regex[i + 1] == '(');
}
inline std::string postfix(const std::string &regex) {
  std::string regex2;
  bool escaped = false;
  for (int i = 0; i < regex.size(); i++) {
    regex2.push_back(regex[i]);
    if (i < regex.size() - 1 && concat_valid(regex, i, escaped)) {
      regex2.push_back('.');
    }
    if (!escaped) {
      if (regex[i] == '\\') {
        escaped = true;
      }
    } else {
      escaped = false;
    }
  }
  std::string ret;
  std::stack<char> stack;
  escaped = false;
  for (int i = 0; i < regex2.size(); i++) {
    if (regex2[i] == '\\' && !escaped) {
      ret.push_back('\\');
    } else if (escaped || !is_operator(regex2[i])) {
      ret.push_back(regex2[i]);
    } else {
      if (regex2[i] == '(') {
        stack.push('(');
      } else if (regex2[i] == ')') {
        while (!stack.empty() && stack.top() != '(') {
          char x = stack.top();
          stack.pop();
          ret.push_back(x);
        }
        if (!stack.empty()) {
          stack.pop();
        }
      } else {
        if (regex2[i] == '*') {
          ret.push_back('*');
        } else {
          while (!stack.empty() && stack.top() == '.') {
            char x = stack.top();
            stack.pop();
            ret.push_back(x);
          }
          stack.push(regex2[i]);
        }
      }
    }
    if (!escaped) {
      if (regex2[i] == '\\') {
        escaped = true;
      }
    } else {
      escaped = false;
    }
  }
  while (!stack.empty()) {
    char x = stack.top();
    stack.pop();
    ret.push_back(x);
  }
  return ret;
}
inline NFA regex_to_nfa(const std::string &regex) {
  if (regex.empty()) {
    std::vector<std::vector<std::vector<int>>> delta(
        1, std::vector<std::vector<int>>(ALPHABET_SIZE + 1));
    std::vector<Token> token_types(1);
    token_types[0] = Token::TOK_FINAL_PLACEHOLDER;
    return NFA(1, 0, delta, token_types);
  }
  std::string postfix_regex = postfix(regex);
  std::stack<NFA> s;
  bool escaped = false;
  for (int i = 0; i < postfix_regex.size(); i++) {
    if (is_operator(postfix_regex[i]) && !escaped) {
      if (postfix_regex[i] == '*') {
        NFA nfa1 = s.top();
        s.pop();
        NFA nfa2 = nfa1.kleene_star();
        s.push(nfa2);
      } else {
        NFA nfa2 = s.top();
        s.pop();
        NFA nfa1 = s.top();
        s.pop();
        if (postfix_regex[i] == '+') {
          s.push(nfa1.union_nfa(nfa2));
        } else {
          s.push(nfa1.concat_nfa(nfa2));
        }
      }
    } else {
      if (postfix_regex[i] == 'e' && escaped) {
        std::vector<Token> token_types2(1, Token::TOK_FINAL_PLACEHOLDER);
        std::vector<std::vector<std::vector<int>>> delta2(
            1, std::vector<std::vector<int>>(ALPHABET_SIZE + 1));
        NFA nfa2(1, 0, delta2, token_types2);
        s.push(nfa2);
      } else if (postfix_regex[i] == '?' && escaped) {
        std::vector<Token> token_types2(1, Token::TOK_FINAL_PLACEHOLDER);
        std::vector<std::vector<std::vector<int>>> delta2(
            1, std::vector<std::vector<int>>(ALPHABET_SIZE + 1));
        for (int alpha = 0; alpha < ALPHABET_SIZE; alpha++) {
          if (alpha != '\n') {
            delta2[0][static_cast<unsigned char>(alpha)].push_back(0);
          }
        }
        NFA nfa2(1, 0, delta2, token_types2);
        s.push(nfa2);
      } else if (postfix_regex[i] != '\\' || escaped) {
        std::vector<Token> token_types2(2, Token::TOK_ERROR);
        token_types2[1] = Token::TOK_FINAL_PLACEHOLDER;
        std::vector<std::vector<std::vector<int>>> delta2(
            2, std::vector<std::vector<int>>(ALPHABET_SIZE + 1));
        delta2[0][static_cast<unsigned char>(postfix_regex[i])].push_back(1);
        NFA nfa2(2, 0, delta2, token_types2);
        s.push(nfa2);
      }
    }
    if (!escaped) {
      if (postfix_regex[i] == '\\') {
        escaped = true;
      }
    } else {
      escaped = false;
    }
  }
  NFA nfa_final = s.top();
  s.pop();
  return nfa_final;
}
inline DFA regex_to_dfa(const std::vector<std::string> &regexes,
                        const std::vector<Token> &tokens) {
  std::vector<std::vector<std::vector<int>>> delta(
      1, std::vector<std::vector<int>>(ALPHABET_SIZE + 1));
  std::vector<Token> token_types(1, Token::TOK_ERROR);
  NFA nfa(1, 0, delta, token_types);
  for (int i = 0; i < regexes.size(); i++) {
    NFA nfa_current = regex_to_nfa(regexes[i]);
    for (int state = 0; state < nfa_current.num_states; state++) {
      if (nfa_current.token_types[state] == Token::TOK_FINAL_PLACEHOLDER) {
        nfa_current.token_types[state] = tokens[i];
      }
    }
    nfa = nfa.union_nfa(nfa_current);
  }
  return nfa.to_dfa().minimize();
}
