#pragma once
#include "box.hpp"
#include "call_frame.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "function.hpp"
#include "string_intern.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#include <time.h>

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

  static Value clockNative(int argCount, std::vector<Value>::iterator args) {
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;
  }

  [[nodiscard]] inline const bool isFalsey(const Value &val);
  [[nodiscard]] inline const bool valuesEqual(const Value &a, const Value &b);
  [[nodiscard]] const bool callValue(const Value &callee,
                                     const uint8_t argCount);
  [[nodiscard]] const UpvalueObj captureUpvalue(const std::vector<Value>::iterator &local);
  [[nodiscard]] const bool call(const box<Closure> closure,
                                const uint8_t argCount);

  void defineNative(std::string name, NativeFunction fn);
  template <typename... Args>
  void runtimeError(const std::string_view format, Args &&...args) {

    std::cout << fmt::vformat(format, fmt::make_format_args(args...)) << '\n';
    for (int i = frames.size() - 1; i >= 0; i--) {
      size_t line = frames[i].closure.function->chunk.getLine(frames[i].ip());
      std::cout << fmt::format("[line {}] in script\n", line);
      if (frames[i].closure.function->name.empty()) {
        std::cout << "script\n";
      } else {
        std::cout << frames[i].closure.function->name << "\n";
      }
    }
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
