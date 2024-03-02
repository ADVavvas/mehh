#pragma once
#include "value.hpp"
#include <cstdint>
#include <vector>

enum OpCode : uint8_t {
  OP_CONSTANT,
  OP_RETURN,
};

class Chunk {
public:
  // TODO: What type? Implicitly converted from size_t?
  void write(uint8_t byte) noexcept;
  size_t writeConstant(Value &&value) noexcept;
  [[nodiscard]] const std::vector<uint8_t> &getCode() const noexcept;
  [[nodiscard]] const ValueArray &getConstants() const noexcept;
  [[nodiscard]] const size_t count() const noexcept;

private:
  std::vector<uint8_t> code;
  ValueArray constants;
};