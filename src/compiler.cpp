#include "compiler.hpp"
#include "chunk.hpp"
#include "precedence.hpp"
#include "token.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

const std::optional<Chunk>
Compiler::compile(const std::string_view source) noexcept {
  scanner.init(source);
  Chunk chunk{};
  compilingChunk = &chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  expression();
  consume(TokenType::END_OF_FILE, "Expect end of expression");
  Chunk c;
  endCompiler();
  if (!parser.hadError) {
    return std::make_optional(c);
  }
  return std::nullopt_t;
}

Chunk &Compiler::currentChunk() const noexcept { return *compilingChunk; }

void Compiler::advance() noexcept {
  parser.previous = parser.current;
  while (1) {
    parser.current = scanToken();
    if (parser.current.type != TokenType::ERROR) {
      break;
    }

    errorAtCurrent(parser.current.lexeme);
  }
}

void Compiler::consume(const TokenType type,
                       const std::string_view message) noexcept {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

void Compiler::errorAt(const Token &token,
                       const std::string_view message) noexcept {
  if (parser.panicMode) {
    return;
  }
  parser.panicMode = true;
  std::cout << "[line " << token.line << "] Error";
  if (token.type == TokenType::END_OF_FILE) {
    std::cout << "at end";
  } else if (token.type == TokenType::ERROR) {
    // nothing
  } else {
    std::cout << " at " << token.lexeme;
  }

  std::cout << ": " << message << '\n';
  parser.hadError = true;
}

void Compiler::errorAtCurrent(const std::string_view message) noexcept {
  errorAt(parser.current, message);
}

void Compiler::error(const std::string_view message) noexcept {
  errorAt(parser.previous, message);
}

void Compiler::endCompiler() noexcept { emitReturn(); }

uint8_t Compiler::makeConstant(const Value &value) noexcept {
  size_t constant = currentChunk().writeConstant(value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return static_cast<uint8_t>(constant);
}

void Compiler::emitByte(uint8_t byte) noexcept {
  currentChunk().write(byte, parser.previous.line);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) noexcept {
  emitByte(byte1);
  emitByte(byte2);
}

void Compiler::emitReturn() noexcept { emitByte(OpCode::OP_RETURN); }

void Compiler::emitConstant(const Value &value) noexcept {
  emitBytes(OpCode::OP_CONSTANT, makeConstant(value));
}

void Compiler::parsePrecedence(const Precedence precedence) noexcept {};

void Compiler::expression() noexcept {
  parsePrecedence(Precedence::ASSIGNMENT);
}

void Compiler::number() noexcept {
  double value = std::stod(parser.previous.lexeme.data());
  emitConstant(value);
}

void Compiler::grouping() noexcept {
  expression();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after expression");
}

void Compiler::unary() noexcept {
  TokenType operatorType = parser.previous.type;

  parsePrecedence(Precedence::UNARY);

  switch (operatorType) {
  case TokenType::MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    // Should be unreachable.
    return;
  }
}