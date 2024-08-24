#pragma once

#include "boost/container/static_vector.hpp"
#include "boost/unordered/unordered_map.hpp"
#include <absl/container/flat_hash_map.h>
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
#include <iterator>
#include <string>
#include <string_view>
#include <time.h>
#include <variant>
#include <vector>

#define FRAME_MAX 64
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
  NativeFunction native = NativeFunction{VM::clockNative};
  boost::container::static_vector<Closure, STACK_MAX> closures;
  boost::container::static_vector<Value, STACK_MAX> stack;
  boost::container::static_vector<CallFrame, FRAME_MAX> frames;
  std::vector<uint8_t>::const_iterator ip;
  absl::flat_hash_map<std::string_view, Value> globals;
  const Chunk *chunk;
  StringIntern stringIntern;
  Compiler compiler;

  static Value clockNative(int argCount, Value *args) {
    return static_cast<double>(clock()) / CLOCKS_PER_SEC;
  }

  [[nodiscard]] inline const bool isFalsey(const Value &val);
  [[nodiscard]] inline const bool valuesEqual(const Value &a, const Value &b);
  [[nodiscard]] const bool callValue(const Value &callee,
                                     const uint8_t argCount);
  [[nodiscard]] const UpvalueObj captureUpvalue(const StackIterator &local);
  [[nodiscard]] const bool call(const Closure *closure, const uint8_t argCount);

  void defineNative(std::string name, NativeFunction *fn);
  template <typename... Args>
  void runtimeError(const std::string_view format, Args &&...args) {
    std::cout << fmt::vformat(format, fmt::make_format_args(args...)) << '\n';
    for (int i = frames.size() - 1; i >= 0; i--) {
      // TODO: use iterator directly
      size_t line = frames[i].closure->function->chunk->getLine(std::distance(frames[i].closure->function->chunk->code().begin(), frames[i].ip()));
      std::cout << fmt::format("[line {}] in script\n", line);
      if (frames[i].closure->function->name.empty()) {
        std::cout << "script\n";
      } else {
        std::cout << frames[i].closure->function->name << "\n";
      }
    }
  }

  template <typename BinaryOperation>
  __attribute__ ((always_inline)) inline InterpretResult binary_op(BinaryOperation &&op) {
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
