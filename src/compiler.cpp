#include "compiler.hpp"
#include "chunk.hpp"
#include "common.hpp"
#include "precedence.hpp"
#include "token.hpp"
#include <_types/_uint8_t.h>
#include <cstddef>
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

void Compiler::beginScope() noexcept { scopeDepth++; }

void Compiler::endScope() noexcept {
  scopeDepth--;
  while (!locals.empty() && locals.back().depth > scopeDepth) {
    emitByte(OP_POP);
    locals.pop_back();
  }
}

void Compiler::addLocal(const Token &token) noexcept {
  // TODO: Enforce max depth.
  locals.push_back({token, UINT8_MAX});
}

uint8_t Compiler::resolveLocal(const Token &token) noexcept {
  for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
    if (it->name.lexeme == token.lexeme) {
      if (it->depth == UINT8_MAX) {
        error("Cannot read local variable in its own initializer.");
      }
      return static_cast<uint8_t>(std::distance(it, locals.rend()) - 1);
    }
  }
  return UINT8_MAX;
}

void Compiler::markInitialized() noexcept { locals.back().depth = scopeDepth; }

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

size_t Compiler::emitJump(const uint8_t instruction) noexcept {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk().count() - 2;
}

void Compiler::emitLoop(const size_t loopStart) noexcept {
  emitByte(OpCode::OP_LOOP);
  size_t offset = currentChunk().count() - loopStart + 2;
  if (offset > UINT16_MAX) {
    error("Loop body too large.");
  }

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
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

  declareVariable();
  if (scopeDepth > 0) {
    return 0;
  }

  return identifierConstant(parser.previous);
}

const uint8_t Compiler::identifierConstant(const Token &name) noexcept {
  std::string_view interned = stringIntern.intern(parser.previous.lexeme);
  return makeConstant(interned);
}

void Compiler::defineVariable(uint8_t global) noexcept {
  if (scopeDepth > 0) {
    markInitialized();
    return;
  }
  emitBytes(OpCode::OP_DEFINE_GLOBAL, global);
}

void Compiler::declareVariable() noexcept {
  if (scopeDepth == 0) {
    return;
  }

  const Token name = parser.previous;
  for (auto rit = locals.rbegin(); rit != locals.rend(); ++rit) {
    if (rit->depth != UINT8_MAX && rit->depth < scopeDepth) {
      break;
    }

    if (rit->name.lexeme == name.lexeme) {
      error("Variable with this name already declared in this scope.");
    }
  }
  addLocal(name);
}

void Compiler::namedVariable(const Token &name) noexcept {
  uint8_t getOp, setOp;
  uint8_t arg = resolveLocal(name);
  if (arg != UINT8_MAX) {
    getOp = OpCode::OP_GET_LOCAL;
    setOp = OpCode::OP_SET_LOCAL;
  } else {
    arg = identifierConstant(name);
    getOp = OpCode::OP_GET_GLOBAL;
    setOp = OpCode::OP_SET_GLOBAL;
  }
  if (canAssign && match(TokenType::EQUAL)) {
    expression();
    emitBytes(setOp, arg);
  } else {
    emitBytes(getOp, arg);
  }
}

void Compiler::patchJump(size_t offset) noexcept {
  // -2 to adjust for the bytecode for the jump offset itself.
  size_t jump = currentChunk().count() - offset - 2;
  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  currentChunk().code()[offset] = (jump >> 8) & 0xff;
  currentChunk().code()[offset + 1] = jump & 0xff;
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

void Compiler::and_() noexcept {
  size_t endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(Precedence::AND);
  patchJump(endJump);
}

void Compiler::or_() noexcept {
  size_t elseJump = emitJump(OP_JUMP_IF_FALSE);
  size_t endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(Precedence::OR);
  patchJump(endJump);
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

void Compiler::block() noexcept {
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    declaration();
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
}

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
  } else if (match(TokenType::WHILE)) {
    whileStatement();
  } else if (match(TokenType::FOR)) {
    forStatement();
  } else if (match(TokenType::LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (match(TokenType::IF)) {
    ifStatement();
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

void Compiler::ifStatement() noexcept {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'");
  expression();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after condition");

  size_t thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  size_t elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TokenType::ELSE)) {
    statement();
  }
  patchJump(elseJump);
}

void Compiler::whileStatement() noexcept {
  size_t loopStart = currentChunk().count();
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'");
  expression();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after condition");

  size_t exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);

  statement();
  emitLoop(loopStart);
  patchJump(exitJump);
  emitByte(OP_POP);
}

void Compiler::forStatement() noexcept {
  beginScope();
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'");
  if (match(TokenType::SEMICOLON)) {
    // No initializer.
  } else if (match(TokenType::VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  size_t loopStart = currentChunk().count();
  size_t exitJump = 0;
  if (!match(TokenType::SEMICOLON)) {
    expression();
    consume(TokenType::SEMICOLON, "Expect ';' after for loop condition");
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
  }

  // Increment statement.
  if (!match(TokenType::RIGHT_PAREN)) {
    size_t bodyJump = emitJump(OP_JUMP);
    size_t incrementStart = currentChunk().count();
    expression();
    emitByte(OP_POP);
    consume(TokenType::RIGHT_PAREN, "Expect ')' after for loop increment");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != 0) {
    patchJump(exitJump);
    emitByte(OP_POP);
  }
  endScope();
}

const ParseRule Compiler::getRule(const TokenType type) const {
  return rules[static_cast<std::underlying_type<TokenType>::type>(type)];
}
