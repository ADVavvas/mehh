#pragma once
#include "token.hpp"
#include <string_view>

class Scanner {

public:
  explicit Scanner(const std::string_view source) noexcept;
  [[nodiscard]] const Token scanToken() noexcept;

private:
  size_t start;
  size_t current;
  size_t line;
  [[nodiscard]] const Token makeToken(TokenType) const noexcept;
  [[nodiscard]] const Token
  errorToken(const std::string_view message) const noexcept;
  [[nodiscard]] const bool match(const char expected) noexcept;
  [[nodiscard]] const bool isAtEnd() noexcept;
  [[nodiscard]] const char peek() noexcept;
  [[nodiscard]] const char peekNext() noexcept;
  [[nodiscard]] const Token handleString() noexcept;
  [[nodiscard]] const Token handleNumber() noexcept;
  [[nodiscard]] const Token handleIdentifier() noexcept;
  [[nodiscard]] const TokenType identifyIdentifierType() noexcept;
  [[nodiscard]] const TokenType checkKeyword(const size_t start,
                                             const std::string_view rest,
                                             const TokenType type);
  const char advance() noexcept;
  void skipWhitespace() noexcept;
  std::string_view source;
};