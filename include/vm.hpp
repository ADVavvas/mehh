#pragma once
#include "chunk.hpp"
#include <cstdint>

#define STACK_MAX 256

enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
public:
  VM() noexcept;
  [[nodiscard]] const InterpretResult interpret(const Chunk &chunk);
  const InterpretResult run();

private:
  const Chunk *chunk;
  std::vector<uint8_t>::const_iterator ip;
  std::vector<Value> stack;

  [[nodiscard]] inline const uint8_t readByte();
  [[nodiscard]] inline const Value readConstant();

  template <typename BinaryOperation> void binary_op(BinaryOperation &&op) {
    Value b = stack.back();
    stack.pop_back();
    Value a = stack.back();
    stack.pop_back();
    stack.push_back(op(a, b));
  }
};