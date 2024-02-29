#pragma once
#include <cstdint>
#include <vector>

enum OpCode : uint8_t {
  OP_RETURN,
};

class Chunk {
public:
  void write(OpCode opcode) noexcept;
  [[nodiscard]] const std::vector<OpCode> &codes() const noexcept;
  [[nodiscard]] const size_t count() const noexcept;

private:
  std::vector<OpCode> code;
};