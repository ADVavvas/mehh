#pragma once
#include "value.hpp"
#include "value_array.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

enum OpCode : uint8_t {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_DEFINE_GLOBAL,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,
  OP_RETURN,
};

struct Line {
  size_t count;
  size_t line;
};

class Chunk {
public:
  // TODO: What type? Implicitly converted from size_t?
  void write(uint8_t byte, size_t line) noexcept;
  size_t writeConstant(const Value &value) noexcept;
  [[nodiscard]] const std::vector<uint8_t> &getCode() const noexcept;
  [[nodiscard]] std::vector<uint8_t> &code() noexcept;
  [[nodiscard]] const uint8_t getLine(size_t offset) const noexcept;
  [[nodiscard]] const std::vector<Line> &getLines() const noexcept;
  [[nodiscard]] const ValueArray &getConstants() const noexcept;
  [[nodiscard]] const size_t count() const noexcept;

private:
  ValueArray constants;
  std::vector<uint8_t> code_;
  std::vector<Line> lines;
};
