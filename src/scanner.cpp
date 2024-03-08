#include "scanner.hpp"
#include "token.hpp"
#include <cctype>

void Scanner::init(const std::string_view source) noexcept {
  this->source = source;
  start = 0;
  current = 0;
  line = 1;
}
const Token Scanner::scanToken() noexcept {
  skipWhitespace();
  start = current;
  if (isAtEnd()) {
    return makeToken(TokenType::END_OF_FILE);
  }

  const char c = advance();

  switch (c) {
  case '(':
    return makeToken(TokenType::LEFT_PAREN);
  case ')':
    return makeToken(TokenType::RIGHT_PAREN);
  case '{':
    return makeToken(TokenType::LEFT_BRACE);
  case '}':
    return makeToken(TokenType::RIGHT_BRACE);
  case ';':
    return makeToken(TokenType::SEMICOLON);
  case ',':
    return makeToken(TokenType::COMMA);
  case '.':
    return makeToken(TokenType::DOT);
  case '-':
    return makeToken(TokenType::MINUS);
  case '+':
    return makeToken(TokenType::PLUS);
  case '/':
    return makeToken(TokenType::SLASH);
  case '*':
    return makeToken(TokenType::STAR);
  case '!':
    return makeToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
  case '=':
    return makeToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
  case '<':
    return makeToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
  case '>':
    return makeToken(match('=') ? TokenType::GREATER_EQUAL
                                : TokenType::GREATER);
  case '"':
    return handleString();
  default:
    if (std::isdigit(c)) {
      return handleNumber();
    }

    if (std::isalpha(c)) {
      return handleIdentifier();
    }
  }

  return Token{TokenType::ERROR, "Error message", line};
}

const bool Scanner::match(const char expected) noexcept {
  if (isAtEnd()) {
    return false;
  }
  if (source[current++] != expected)
    return false;
  return true;
}

const Token Scanner::handleString() noexcept {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      ++line;
    }
    advance();
  }
  if (isAtEnd()) {
    return errorToken("Unterminated string");
  }

  // Closing quote
  advance();
  return makeToken(TokenType::STRING);
}

const Token Scanner::handleNumber() noexcept {
  while (std::isdigit(peek())) {
    advance();
    if (peek() == '.' && std::isdigit(peekNext())) {
      advance();
      while (std::isdigit(peek())) {
        advance();
      }
    }
  }
  return makeToken(TokenType::NUMBER);
}

const Token Scanner::handleIdentifier() noexcept {
  while (std::isalpha(peek()) || std::isdigit(peek())) {
    advance();
  }
  return makeToken(identifyIdentifierType());
}

const TokenType Scanner::identifyIdentifierType() noexcept {
  switch (source[start]) {
  case 'a':
    return checkKeyword(1, "nd", TokenType::AND);
  case 'c':
    return checkKeyword(1, "lass", TokenType::CLASS);
  case 'e':
    return checkKeyword(1, "lse", TokenType::ELSE);
  case 'f':
    if (current - start > 1) {
      switch (source[start + 1]) {
      case 'a':
        return checkKeyword(2, "lse", TokenType::FALSE);
      case 'o':
        return checkKeyword(2, "r", TokenType::FOR);
      case 'u':
        return checkKeyword(2, "n", TokenType::FUN);
      }
    }
    break;
  case 'i':
    return checkKeyword(1, "f", TokenType::IF);
  case 'n':
    return checkKeyword(1, "il", TokenType::NIL);
  case 'o':
    return checkKeyword(1, "r", TokenType::OR);
  case 'p':
    return checkKeyword(1, "rint", TokenType::PRINT);
  case 'r':
    return checkKeyword(1, "eturn", TokenType::RETURN);
  case 's':
    return checkKeyword(1, "uper", TokenType::SUPER);
  case 't':
    if (current - start > 1) {
      switch (source[start + 1]) {
      case 'h':
        return checkKeyword(2, "is", TokenType::THIS);
      case 'r':
        return checkKeyword(2, "ue", TokenType::TRUE);
      }
    }
    break;
  case 'v':
    return checkKeyword(1, "ar", TokenType::VAR);
  case 'w':
    return checkKeyword(1, "hile", TokenType::WHILE);
  }
  return TokenType::IDENTIFIER;
}

const TokenType Scanner::checkKeyword(const size_t start,
                                      const std::string_view rest,
                                      const TokenType type) {
  if (rest.compare(source.substr(this->start + start, rest.size())) == 0) {
    return type;
  }
  return TokenType::IDENTIFIER;
}

const Token Scanner::makeToken(TokenType type) const noexcept {
  return Token{type, source.substr(start, current - start), line};
}
const Token Scanner::errorToken(const std::string_view message) const noexcept {
  return Token{TokenType::ERROR, message, line};
}

const char Scanner::peek() noexcept { return source[current]; }

const char Scanner::peekNext() noexcept {
  if (isAtEnd())
    return '\0';
  return source[current + 1];
}

const char Scanner::advance() noexcept { return source[current++]; }

const bool Scanner::isAtEnd() noexcept { return current != source.size(); }

void Scanner::skipWhitespace() noexcept {
  while (1) {
    const char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      ++line;
      advance();
      break;
    case '/':
      if (peekNext() == '/') {
        // Comment
        while (peek() != '\n' && !isAtEnd()) {
          advance();
        }
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}