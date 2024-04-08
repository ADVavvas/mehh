#include "compiler.hpp"
#include "chunk.hpp"
#include "common.hpp"
#include "precedence.hpp"
#include "token.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#ifdef DEBUG_PRINT_CODE
#include "debug.hpp"
#endif

const std::optional<Chunk>
Compiler::compile(const std::string_view source) noexcept {
  scanner.init(source);
  Chunk chunk{};
  compilingChunk = &chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();
  while (!match(TokenType::END_OF_FILE)) {
    declaration();
  }
  endCompiler();
  if (!parser.hadError) {
    return std::make_optional(chunk);
  }
  return std::nullopt;
}

void Compiler::synchronize() {
  parser.panicMode = false;
  // Skip tokens until we find a semicolon (end of statement) or a token that
  // starts a statement.
  while (parser.current.type != TokenType::END_OF_FILE) {
    if (parser.previous.type == TokenType::SEMICOLON) {
      return;
    }
    switch (parser.current.type) {
    case TokenType::CLASS:
    case TokenType::FUN:
    case TokenType::VAR:
    case TokenType::FOR:
    case TokenType::IF:
    case TokenType::WHILE:
    case TokenType::PRINT:
    case TokenType::RETURN:
      return;
    default:;
    }
  }
}

Chunk &Compiler::currentChunk() const noexcept { return *compilingChunk; }

[[nodiscard]] inline bool Compiler::check(const TokenType type) noexcept {
  return parser.current.type == type;
}

[[nodiscard]] bool Compiler::match(const TokenType type) noexcept {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

void Compiler::advance() noexcept {
  parser.previous = parser.current;
  while (1) {
    parser.current = scanner.scanToken();
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
    std::cout << " at end";
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

void Compiler::endCompiler() noexcept {
  emitReturn();
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

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

void Compiler::parsePrecedence(const Precedence precedence) noexcept {
  advance();
  const ParseFn prefixRule = getRule(parser.previous.type).prefix;
  if (prefixRule == nullptr) {
    error("Expect expression");
    return;
  }

  canAssign = precedence <= Precedence::ASSIGNMENT;
  (*this.*prefixRule)();

  while (precedence <= getRule(parser.current.type).precedence) {
    advance();
    const ParseFn infixRule = getRule(parser.previous.type).infix;
    (*this.*infixRule)();
  }
  if (canAssign && match(TokenType::EQUAL)) {
    error("Invalid assignment target.");
  }
};

const uint8_t
Compiler::parseVariable(const std::string_view errorMessage) noexcept {
  consume(TokenType::IDENTIFIER, errorMessage);
  return identifierConstant(parser.previous);
}

const uint8_t Compiler::identifierConstant(const Token &name) noexcept {
  std::string_view interned = stringIntern.intern(parser.previous.lexeme);
  return makeConstant(interned);
}

void Compiler::defineVariable(uint8_t global) noexcept {
  emitBytes(OpCode::OP_DEFINE_GLOBAL, global);
}

void Compiler::namedVariable(const Token &name) noexcept {
  const uint8_t arg = identifierConstant(name);
  if (canAssign && match(TokenType::EQUAL)) {
    expression();
    emitBytes(OpCode::OP_SET_GLOBAL, arg);
  } else {
    emitBytes(OpCode::OP_GET_GLOBAL, arg);
  }
}

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
  case TokenType::BANG:
    emitByte(OP_NOT);
    break;
  case TokenType::MINUS:
    emitByte(OP_NEGATE);
    break;
  default:
    // Should be unreachable.
    return;
  }
}

void Compiler::binary() noexcept {
  TokenType operatorType = parser.previous.type;

  const ParseRule rule = getRule(operatorType);
  parsePrecedence(rule.nextPrecedence());

  switch (operatorType) {
  case TokenType::PLUS:
    emitByte(OpCode::OP_ADD);
    break;
  case TokenType::MINUS:
    emitByte(OpCode::OP_SUBTRACT);
    break;
  case TokenType::STAR:
    emitByte(OpCode::OP_MULTIPLY);
    break;
  case TokenType::SLASH:
    emitByte(OpCode::OP_DIVIDE);
    break;
  case TokenType::BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TokenType::EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TokenType::GREATER:
    emitByte(OP_GREATER);
    break;
  case TokenType::GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TokenType::LESS:
    emitByte(OP_LESS);
    break;
  case TokenType::LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  default:
    // Should be unreachable.
    return;
  }
}

// TODO: See diff with function per literal instead of switch.
void Compiler::literal() noexcept {
  switch (parser.previous.type) {
  case TokenType::FALSE:
    emitByte(OP_FALSE);
    break;
  case TokenType::NIL:
    emitByte(OP_NIL);
    break;
  case TokenType::TRUE:
    emitByte(OP_TRUE);
    break;
  default:
    break;
  }
}

void Compiler::string() noexcept {
  if (parser.previous.lexeme.size() < 2) {
    return;
  }
  std::string_view interned = stringIntern.intern(
      parser.previous.lexeme.substr(1, parser.previous.lexeme.size() - 2));
  emitConstant(interned);
}

void Compiler::variable() noexcept { namedVariable(parser.previous); }

void Compiler::declaration() noexcept {
  if (match(TokenType::VAR)) {
    varDeclaration();
  } else {
    statement();
  }
  if (parser.panicMode) {
    synchronize();
  }
}

void Compiler::varDeclaration() noexcept {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TokenType::EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
  defineVariable(global);
}

void Compiler::statement() noexcept {
  if (match(TokenType::PRINT)) {
    printStatement();
  } else {
    expressionStatement();
  }
}

void Compiler::printStatement() noexcept {
  expression();
  consume(TokenType::SEMICOLON, "Expected ';' after expression");
  emitByte(OP_PRINT);
}

void Compiler::expressionStatement() noexcept {
  expression();
  consume(TokenType::SEMICOLON, "Expected ';' after expression");
  emitByte(OP_POP);
}

const ParseRule Compiler::getRule(const TokenType type) const {
  return rules[static_cast<std::underlying_type<TokenType>::type>(type)];
}
