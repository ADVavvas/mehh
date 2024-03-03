#pragma once
#include "chunk.hpp"
#include <cstdint>

enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
public:
  [[nodiscard]] const InterpretResult interpret(const Chunk &chunk);
  const InterpretResult run();

private:
  const Chunk *chunk;
  std::vector<uint8_t>::const_iterator ip;

  [[nodiscard]] inline const uint8_t readByte();
  [[nodiscard]] inline const Value readConstant();
};