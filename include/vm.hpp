#pragma once

#include "boost/container/static_vector.hpp"
#include "boost/unordered/unordered_map.hpp"
#include "call_frame.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "function.hpp"
#include "string_intern.hpp"
#include "value.hpp"
#include <absl/container/flat_hash_map.h>
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
  InterpretResult dispatch();

private:
  CallFrame *frame;
  NativeFunction native = NativeFunction{VM::clockNative};
  boost::container::static_vector<Closure, STACK_MAX> closures;
  boost::container::static_vector<Value, STACK_MAX> stack;
  boost::container::static_vector<StringObj, STACK_MAX> strings; // TODO: Temp - Fixme
  boost::container::static_vector<CallFrame, FRAME_MAX> frames;
  std::vector<uint8_t>::const_iterator ip;
  absl::flat_hash_map<std::string_view, Value> globals;
  const Chunk *chunk;
  StringIntern stringIntern;
  Compiler compiler;
  InterpretResult op_return();
  InterpretResult op_call();
  InterpretResult op_subtract();
  InterpretResult op_constant();
  InterpretResult op_less();
  InterpretResult op_add();
  InterpretResult op_negate();
  InterpretResult op_nil();
  InterpretResult op_true();
  InterpretResult op_false();
  InterpretResult op_multiply();
  InterpretResult op_pop();
  InterpretResult op_get_global();
  InterpretResult op_set_global();
  InterpretResult op_get_local();
  InterpretResult op_set_local();
  InterpretResult op_get_upvalue();
  InterpretResult op_set_upvalue();
  InterpretResult op_define_global();
  InterpretResult op_equal();
  InterpretResult op_greater();
  InterpretResult op_divide();
  InterpretResult op_not();
  InterpretResult op_print();
  InterpretResult op_jump();
  InterpretResult op_jump_if_false();
  InterpretResult op_loop();
  InterpretResult op_closure();

  static Value clockNative(int argCount, Value *args) {
    return Value{static_cast<double>(clock()) / CLOCKS_PER_SEC};
  }

  [[nodiscard]] inline const bool isFalsey(const Value &val);
  [[nodiscard]] inline const bool valuesEqual(const Value &a, const Value &b);
  [[nodiscard]] const bool callValue(const Value &callee,
                                     const uint8_t argCount);
  [[nodiscard]] const UpvalueObj captureUpvalue(const StackIterator &local);
  [[nodiscard]] const bool call(const Closure *closure, const uint8_t argCount);

  void defineNative(std::string name, NativeFunction *fn);
  template <typename... Args>
  __attribute__((always_inline)) inline
  void runtimeError(const std::string_view format, Args &&...args) {
    std::cout << fmt::vformat(format, fmt::make_format_args(args...)) << '\n';
    for (int i = frames.size() - 1; i >= 0; i--) {
      // TODO: use iterator directly
      size_t line = frames[i].closure->function->chunk->getLine(std::distance(
          frames[i].closure->function->chunk->code().begin(), frames[i].ip()));
      std::cout << fmt::format("[line {}] in script\n", line);
      if (frames[i].closure->function->name.empty()) {
        std::cout << "script\n";
      } else {
        std::cout << frames[i].closure->function->name << "\n";
      }
    }
  }

  template <typename BinaryOperation>
  __attribute__((always_inline)) inline InterpretResult
  binary_op(BinaryOperation &&op) {
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
