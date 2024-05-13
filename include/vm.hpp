#pragma once
#include "chunk.hpp"
#include "compiler.hpp"
#include "string_intern.hpp"
#include <cstdint>
#include <fmt/core.h>
#include <iostream>
#include <set>
#include <string_view>
#include <variant>

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
  StringIntern stringIntern;
  Compiler compiler;
  std::vector<uint8_t>::const_iterator ip;
  std::vector<Value> stack;
  std::unordered_map<std::string_view, Value> globals;

  [[nodiscard]] inline const uint8_t readByte();
  [[nodiscard]] inline const uint16_t readShort();
  [[nodiscard]] inline const Value readConstant();
  [[nodiscard]] inline const bool isFalsey(const Value &val);
  [[nodiscard]] inline const bool valuesEqual(const Value &a, const Value &b);
  template <typename... Args>
  void runtimeError(const std::string_view format, Args &&...args) {

    std::cout << fmt::vformat(format, fmt::make_format_args(args...)) << '\n';

    size_t instruction = ip - chunk->getCode().begin() - 1;
    size_t line = chunk->getLine(instruction);
    std::cout << fmt::format("[line {}] in script\n", line);
  }

  template <typename BinaryOperation>
  InterpretResult binary_op(BinaryOperation &&op) {
    if (stack.size() < 2 ||
        !std::holds_alternative<double>(*(stack.end() - 1)) ||
        !std::holds_alternative<double>(*(stack.end() - 2))) {
      runtimeError("Operands must be numbers");
      return INTERPRET_RUNTIME_ERROR;
    }
    double b = std::get<double>(stack.back());
    stack.pop_back();
    double a = std::get<double>(stack.back());
    stack.pop_back();
    stack.push_back(op(a, b));
    return INTERPRET_OK;
  }
};