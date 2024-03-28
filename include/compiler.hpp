#pragma once
#include "chunk.hpp"
#include "parser.hpp"
#include "precedence.hpp"
#include "scanner.hpp"
#include "token.hpp"
#include <optional>
#include <string_view>

class Compiler;

using ParseFn = void (Compiler::*)();
class ParseRule {
public:
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
  const Precedence nextPrecedence() const {
    return static_cast<Precedence>(
        (static_cast<int>(precedence) + 1) %
        static_cast<int>(Precedence::NUM_PRECEDENCES));
  }
};

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
  void binary() noexcept;
  void literal() noexcept;
  const ParseRule getRule(const TokenType type) const;

public:
  constexpr static ParseRule rules[40] = {
      {&Compiler::grouping, nullptr, Precedence::NONE},        // LEFT_PAREN
      {nullptr, nullptr, Precedence::NONE},                    // RIGHT_PAREN
      {nullptr, nullptr, Precedence::NONE},                    // LEFT_BRACE
      {nullptr, nullptr, Precedence::NONE},                    // RIGHT_BRACE
      {nullptr, nullptr, Precedence::NONE},                    // COMMA
      {nullptr, nullptr, Precedence::NONE},                    // DOT
      {&Compiler::unary, &Compiler::binary, Precedence::TERM}, // MINUS
      {nullptr, &Compiler::binary, Precedence::TERM},          // PLUS
      {nullptr, nullptr, Precedence::NONE},                    // SEMICOLON
      {nullptr, &Compiler::binary, Precedence::FACTOR},        // SLASH
      {nullptr, &Compiler::binary, Precedence::FACTOR},        // STAR
      {&Compiler::unary, nullptr, Precedence::NONE},           // BANG
      {nullptr, &Compiler::binary, Precedence::EQUALITY},      // BANG_EQUAL
      {nullptr, nullptr, Precedence::NONE},                    // EQUAL
      {nullptr, &Compiler::binary, Precedence::EQUALITY},      // EQUAL_EQUAL
      {nullptr, &Compiler::binary, Precedence::COMPARISON},    // GREATER
      {nullptr, &Compiler::binary, Precedence::COMPARISON},    // GREATER_EQUAL
      {nullptr, &Compiler::binary, Precedence::COMPARISON},    // LESS
      {nullptr, &Compiler::binary, Precedence::COMPARISON},    // LESS_EQUAL
      {nullptr, nullptr, Precedence::NONE},                    // IDENTIFIER
      {nullptr, nullptr, Precedence::NONE},                    // STRING
      {&Compiler::number, nullptr, Precedence::NONE},          // NUMBER
      {nullptr, nullptr, Precedence::NONE},                    // AND
      {nullptr, nullptr, Precedence::NONE},                    // CLASS
      {nullptr, nullptr, Precedence::NONE},                    // ELSE
      {&Compiler::literal, nullptr, Precedence::NONE},         // FALSE
      {nullptr, nullptr, Precedence::NONE},                    // FOR
      {nullptr, nullptr, Precedence::NONE},                    // FUN
      {nullptr, nullptr, Precedence::NONE},                    // IF
      {&Compiler::literal, nullptr, Precedence::NONE},         // NIL
      {nullptr, nullptr, Precedence::NONE},                    // OR
      {nullptr, nullptr, Precedence::NONE},                    // PRINT
      {nullptr, nullptr, Precedence::NONE},                    // RETURN
      {nullptr, nullptr, Precedence::NONE},                    // SUPER
      {nullptr, nullptr, Precedence::NONE},                    // THIS
      {&Compiler::literal, nullptr, Precedence::NONE},         // TRUE
      {nullptr, nullptr, Precedence::NONE},                    // VAR
      {nullptr, nullptr, Precedence::NONE},                    // WHILE
      {nullptr, nullptr, Precedence::NONE},                    // ERROR
      {nullptr, nullptr, Precedence::NONE},                    // EOF
  };
};