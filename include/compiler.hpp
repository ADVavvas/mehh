#pragma once
#include "chunk.hpp"
#include "parser.hpp"
#include "precedence.hpp"
#include "scanner.hpp"
#include "token.hpp"
#include <optional>
#include <string_view>

class Compiler {
public:
  [[nodiscard]] const std::optional<Chunk>
  compile(const std::string_view source) noexcept;

private:
  Scanner scanner;
  Parser parser;
  Chunk *compilingChunk;

  [[nodiscard]] Chunk &currentChunk() const noexcept;
  void advance() noexcept;
  void consume(const TokenType type, const std::string_view message) noexcept;
  void error(const std::string_view message) noexcept;
  void errorAt(const Token &token, const std::string_view message) noexcept;
  void errorAtCurrent(const std::string_view message) noexcept;
  void endCompiler() noexcept;
  inline uint8_t makeConstant(const Value &value) noexcept;
  inline void emitByte(uint8_t byte) noexcept;
  inline void emitBytes(uint8_t byte1, uint8_t byte2) noexcept;
  inline void emitReturn() noexcept;
  inline void emitConstant(const Value &value) noexcept;
  void parsePrecedence(const Precedence precedence) noexcept;
  void expression() noexcept;
  void number() noexcept;
  void grouping() noexcept;
  void unary() noexcept;
};