#include "lexer.hpp"
int main() {
  std::string text =
      "let x: int = 3;\n#this is a comment?\nconst comment_ends = -.3;";
  std::vector<Lexeme> lexemes = maximal_munch(text);
  for (int i = 0; i < lexemes.size() - 1; i++) {
    auto lexeme = lexemes[i];
    std::cout << text.substr(lexeme.start, lexeme.length) << " -> "
              << token_names[static_cast<int>(lexeme.tok)] << "\n";
  }
}
