#pragma once
#include "value.hpp"
#include <cstdint>
#include <vector>

enum OpCode : uint8_t {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
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
  [[nodiscard]] const uint8_t getLine(size_t offset) const noexcept;
  [[nodiscard]] const std::vector<Line> &getLines() const noexcept;
  [[nodiscard]] const ValueArray &getConstants() const noexcept;
  [[nodiscard]] const size_t count() const noexcept;

private:
  std::vector<uint8_t> code;
  std::vector<Line> lines;
  ValueArray constants;
};