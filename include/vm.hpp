#pragma once
#include "chunk.hpp"
#include "compiler.hpp"
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>

#define STACK_MAX 256

enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
public:
  VM() noexcept;
  [[nodiscard]] const InterpretResult interpret(const std::string_view source);
  const InterpretResult run();

private:
  const Chunk *chunk;
  Compiler compiler;
  std::vector<uint8_t>::const_iterator ip;
  std::vector<Value> stack;

  [[nodiscard]] inline const uint8_t readByte();
  [[nodiscard]] inline const Value readConstant();
  template <typename... Args>
  void runtimeError(const std::string_view format, Args &&...args) {

    std::cout << std::vformat(format, std::make_format_args(args...)) << '\n';

    size_t instruction = ip - chunk->getCode().begin() - 1;
    size_t line = chunk->getLine(instruction);
    std::cout << std::format("[line {}] in script\n", line);
  }

  template <typename BinaryOperation> void binary_op(BinaryOperation &&op) {
    Value b = stack.back();
    stack.pop_back();
    Value a = stack.back();
    stack.pop_back();
    stack.push_back(op(a, b));
  }
};