#include "compiler.hpp"
#include "token.hpp"
#include <iostream>

void Compiler::compile(const std::string_view source) const noexcept {
  Scanner scanner{source};
  size_t line = -1;

  while (1) {
    Token token{scanner.scanToken()};
    if (token.line != line) {
      std::cout << token.line;
      line = token.line;
    } else {
      std::cout << "    | ";
    }

    std::cout << token.type << " \'" << token.lexeme << "\'\n";

    if (token.type == TokenType::END_OF_FILE) {
      break;
    }
  }
}