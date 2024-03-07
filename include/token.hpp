#pragma once

#include <cstddef>
#include <string_view>

enum TokenType {
  // Single-character tokens.
  LEFT_PAREN,
  RIGHT_PAREN,
  LEFT_BRACE,
  RIGHT_BRACE,
  COMMA,
  DOT,
  MINUS,
  PLUS,
  SEMICOLON,
  SLASH,
  STAR,
  // One or two character tokens.
  BANG,
  BANG_EQUAL,
  EQUAL,
  EQUAL_EQUAL,
  GREATER,
  GREATER_EQUAL,
  LESS,
  LESS_EQUAL,
  // Literals.
  IDENTIFIER,
  STRING,
  NUMBER,
  // Keywords.
  AND,
  CLASS,
  ELSE,
  FALSE,
  FOR,
  FUN,
  IF,
  NIL,
  OR,
  PRINT,
  RETURN,
  SUPER,
  THIS,
  TRUE,
  VAR,
  WHILE,

  ERROR,
  END_OF_FILE
};

class Token {
public:
  explicit constexpr Token(TokenType type, std::string_view lexeme,
                           size_t line) noexcept
      : type{type}, lexeme{lexeme}, line{line} {};

  TokenType type;
  std::string_view lexeme;
  size_t line;
};