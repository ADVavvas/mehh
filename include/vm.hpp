#pragma once
#include "chunk.hpp"
#include "compiler.hpp"
#include "function.hpp"
#include "string_intern.hpp"
#include "call_frame.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#define FRAME_MAX 64
#define STACK_MAX 256 // Can overflow

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
  std::vector<CallFrame> frames;
  std::unordered_map<std::string_view, Value> globals;

  [[nodiscard]] inline const bool isFalsey(const Value &val);
  [[nodiscard]] inline const bool valuesEqual(const Value &a, const Value &b);
  template <typename... Args>
  void runtimeError(const std::string_view format, Args &&...args) {

    std::cout << fmt::vformat(format, fmt::make_format_args(args...)) << '\n';
    CallFrame &frame = frames.back();

    size_t line = frame.function.chunk.getLine(frame.ip());
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
