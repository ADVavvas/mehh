#include "vm.hpp"
#include "Tracy.hpp"
#include "call_frame.hpp"
#include "chunk.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "function.hpp"
#include "value.hpp"
#include "value_array.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdint.h>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#define BENCHMARK

VM::VM() noexcept : compiler(Compiler{stringIntern}) {
  frames.reserve(STACK_MAX);
  defineNative("clock", &native);
}

inline const bool VM::isFalsey(const Value &val) {
  return std::holds_alternative<nil>(val) ||
         (std::holds_alternative<bool>(val) && !std::get<bool>(val));
}

inline const bool VM::valuesEqual(const Value &a, const Value &b) {
  return std::visit(
      overloaded{
          [](const std::monostate a, const std::monostate b) { return true; },
          [](const bool a, const bool b) { return a == b; },
          [](const double a, const double b) { return a == b; },
          [](const std::string_view a, const std::string_view b) {
            return a == b;
          },
          [](const auto a, const auto b) { return false; },
      },
      a, b);
}

const bool VM::callValue(const Value &callee, const uint8_t argCount) {
  return std::visit(overloaded{
                        [this, argCount](const Closure *val) {
                          ZoneScopedN("std::function");
                          bool res = call(val, argCount);
                          FrameMark;
                          return res;
                        },
                        [this, argCount](const NativeFunction *val) {
                          Value result = val->fun(
                              argCount, stack.end().get_ptr() - argCount);
                          stack.erase(stack.end() - argCount, stack.end());
                          stack.push_back(result);
                          return true;
                        },
                        [this](const auto val) {
                          runtimeError("Can only call functions and classes");
                          return false;
                        },
                    },
                    callee);
}

const UpvalueObj VM::captureUpvalue(const StackIterator &local) {
  return UpvalueObj{local.get_ptr()};
}

const bool VM::call(const Closure *closure, const uint8_t argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected {} arguments but got {}.", closure->function->arity,
                 argCount);
    return false;
  }
  if (frames.size() == FRAME_MAX) {
    runtimeError("Stack overflow");
    return false;
  }
  // (end - argCount - 1) accounts for the 0th slot
  frames.push_back(CallFrame{closure, stack.end() - argCount - 1});
  return true;
}

void VM::defineNative(std::string name, NativeFunction *fn) {
  globals.insert_or_assign(stringIntern.intern(name), fn);
}

const InterpretResult VM::interpret(const std::string_view source) {
  const std::optional<const Function *> &function = compiler.compile(source);
  if (!function.has_value()) {
    return INTERPRET_COMPILE_ERROR;
  }
  const Function *fun = function.value();
  stack.push_back(fun);
  // TODO: This ought to be refactored.
  // Need to be careful here, because we've mixed values, references, pointers
  // and smart pointers.
  const Function *funPtr = std::get<const Function *>(stack.back());
  Closure closure{funPtr};
  if (!call(&closure, 0)) {
    return InterpretResult::INTERPRET_RUNTIME_ERROR;
  }
  return run();
}

const InterpretResult VM::run() {
  ZoneScopedNC("VM run", 0x00FFFF);
  CallFrame *frame = &frames.back();
  uint32_t numInstructions = 0;
#ifdef BENCHMARK
  std::array<int, OP_RETURN + 1> calls{};
  calls.fill(0);
  std::array<uint32_t, OP_RETURN + 1> times{};
  times.fill(0);
#endif
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (auto val : stack) {
      std::cout << "[ ";
      printValue(val);
      std::cout << " ]";
    }
    std::cout << "\n";
    disassembleInstruction(*(frame->closure->function->chunk), frame->getIp());
#endif
    numInstructions++;
    uint8_t instruction;
    switch (instruction = frame->readByte()) {
    case OP_RETURN: {
      ZoneScopedNC("RETURN", tracy::Color::Yellow);
      Value result = std::move(stack.back());
      stack.pop_back();
      frames.pop_back();
      if (frames.empty()) {
        stack.pop_back();
        std::cout << "Instructions: " << numInstructions << '\n';
        std::cout << "Stack capacity: " << stack.capacity() << '\n';
#ifdef BENCHMARK
        std::cout << "SUB: " << calls[OP_SUBTRACT] << '\n';
        std::cout << "Time SUB: " << times[OP_SUBTRACT] << '\n';
        std::cout << "LESS: " << calls[OP_LESS] << '\n';
        std::cout << "Time LESS: " << times[OP_LESS] << '\n';
        std::cout << "GET GLOBAL: " << calls[OP_GET_GLOBAL] << '\n';
        std::cout << "Time GET GLOBAL: " << times[OP_GET_GLOBAL] << '\n';
        std::cout << "GET LOCAL: " << calls[OP_GET_LOCAL] << '\n';
        std::cout << "Time GET LOCAL: " << times[OP_GET_LOCAL] << '\n';
#endif
        return INTERPRET_OK;
        FrameMark;
      }
      // Erase all the called function's stack window.
      stack.erase(frame->slots, stack.end());
      stack.push_back(result);
      frame = &frames.back();
      FrameMark;
      break;
    }
    case OP_CONSTANT: {
      ZoneScopedNC("CONSTANT", tracy::Color::Burlywood);
      const Value &constant = frame->readConstant();
      stack.push_back(constant);
      // printValue(constant);
      // std::cout << "\n";
      FrameMark;
    } break;
    case OP_NIL:
      stack.push_back(nil{});
      break;
    case OP_TRUE:
      stack.push_back(true);
      break;
    case OP_FALSE:
      stack.push_back(false);
      break;
    case OP_NEGATE: {
      Value &val = stack.back();
      if (!std::holds_alternative<double>(val)) {
        runtimeError("Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }
      val = -std::get<double>(val);
    } break;
    case OP_ADD: {
      ZoneScopedNC("ADD", tracy::Color::Brown);
      if (__builtin_expect(stack.size() >= 2, true)) {
        auto b = std::move(stack.back());
        stack.pop_back();
        auto a = std::move(stack.back());
        stack.pop_back();

        const bool success = std::visit(
            overloaded{
                [this](const std::monostate a, const std::monostate b) {
                  runtimeError("Operands must be two numbers or two strings.");
                  return false;
                },
                [this](const bool a, const bool b) {
                  runtimeError("Operands must be two numbers or two strings.");
                  return false;
                },
                [this](const double a, const double b) {
                  stack.push_back(a + b);
                  return true;
                },
                [this](const std::string_view a, const std::string_view b) {
                  std::string_view interned =
                      stringIntern.intern(std::string{a} + std::string{b});
                  stack.push_back(interned);
                  return true;
                },
                [this](const auto a, const auto b) {
                  runtimeError("Operands must be two numbers or two strings.");
                  return false;
                },
            },
            a, b);
        FrameMark;
        if (__builtin_expect(success, true)) {
          break;
        }
      }
      return INTERPRET_RUNTIME_ERROR;
    }
    case OP_SUBTRACT: {
      ZoneScopedNC("SUB", tracy::Color::Crimson);
#ifdef BENCHMARK
      auto t1 = std::chrono::high_resolution_clock::now();
#endif

      if (__builtin_expect(
              stack.size() >= 2 &&
                  std::holds_alternative<double>(*(stack.end() - 1)) &&
                  std::holds_alternative<double>(*(stack.end() - 2)),
              true)) {

        double b = std::get<double>(stack.back());
        stack.pop_back();
        double a = std::get<double>(stack.back());
        stack.pop_back();
        stack.push_back(a - b);
        FrameMark;
#ifdef BENCHMARK
        auto t2 = std::chrono::high_resolution_clock::now();
        /* Getting number of milliseconds as an integer. */
        auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
        times[OP_SUBTRACT] += ms_int.count();
        calls[OP_SUBTRACT]++;
#endif
      } else {
        runtimeError("Operands must be numbers");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_MULTIPLY: {

      auto res = binary_op([](double a, double b) constexpr { return a * b; });
      if (res == INTERPRET_RUNTIME_ERROR) {

        return res;
      }
    } break;
    case OP_DIVIDE: {
      auto res = binary_op([](double a, double b) constexpr { return a / b; });
      if (res == INTERPRET_RUNTIME_ERROR) {
        return res;
      }
      break;
    }
    case OP_NOT: {
      Value &val = stack.back();
      val = isFalsey(val);
      break;
    }
    case OP_EQUAL: {
      Value a = stack.back();
      stack.pop_back();
      Value b = stack.back();
      stack.pop_back();
      stack.push_back(valuesEqual(a, b));
      break;
    }
    case OP_GREATER: {
      auto res = binary_op([](double a, double b) constexpr { return a > b; });
      if (res == INTERPRET_RUNTIME_ERROR) {
        return res;
      }
      break;
    }
    case OP_LESS: {
      ZoneScopedNC("LESS", tracy::Color::Yellow);
#ifdef BENCHMARK
      auto t1 = std::chrono::high_resolution_clock::now();
#endif
      // auto res = binary_op([](double a, double b) constexpr { return a < b;
      // });
      if (__builtin_expect(stack.size() >= 2, true)) {
        if (__builtin_expect(
                std::holds_alternative<double>(*(stack.end() - 1)) &&
                    std::holds_alternative<double>(*(stack.end() - 2)),
                true)) {
          double b = std::get<double>(stack.back());
          stack.pop_back();
          double a = std::get<double>(stack.back());
          stack.pop_back();
          stack.push_back(a < b);
        } else {
          // Handle error or other types
          runtimeError("Operands must be numbers.");
        }
      } else {
        // Handle stack underflow
        runtimeError("Stack underflow.");
      }
#ifdef BENCHMARK
      auto t2 = std::chrono::high_resolution_clock::now();
      /* Getting number of milliseconds as an integer. */
      auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
      times[OP_LESS] += ms_int.count();
      calls[OP_LESS]++;
#endif
      FrameMark;
      // if (res != INTERPRET_RUNTIME_ERROR) {
      //   break;
      // }
      // return res;
      break;
    }
    case OP_PRINT: {
      printValue(stack.back());
      std::cout << "\n";
      stack.pop_back();
      break;
    }
    case OP_DEFINE_GLOBAL: {
      const Value &name = frame->readConstant();
      if (!std::holds_alternative<std::string_view>(name)) {
        break;
      }
      const Value value = stack.back();
      const std::string_view nameStr = std::get<std::string_view>(name);

      globals.insert_or_assign(nameStr, value);
      stack.pop_back();
      break;
    }
    case OP_GET_GLOBAL: {
#ifdef BENCHMARK
      auto t1 = std::chrono::high_resolution_clock::now();
#endif
      const Value &value = frame->readConstant();
      if (std::holds_alternative<std::string_view>(value)) {
        const auto &variable = globals.find(std::get<std::string_view>(value));
        if (variable != globals.end()) {

          stack.push_back(variable->second);
#ifdef BENCHMARK
          auto t2 = std::chrono::high_resolution_clock::now();
          /* Getting number of milliseconds as an integer. */
          auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
          times[OP_GET_GLOBAL] += ms_int.count();
          calls[OP_GET_GLOBAL]++;
#endif
          break;
        } else {
          runtimeError("Undefined variable '{}.'",
                       std::get<std::string_view>(value));
          return INTERPRET_RUNTIME_ERROR;
        }
      }
    }
    case OP_SET_GLOBAL: {
      const Value &name = frame->readConstant();
      if (!std::holds_alternative<std::string_view>(name)) {
        break;
      }
      const Value value = stack.back();
      const std::string_view nameStr = std::get<std::string_view>(name);
      if (!globals.contains(nameStr)) {
        runtimeError("Undefined variable '{}'.", nameStr);
        return INTERPRET_RUNTIME_ERROR;
      }
      globals[std::string{nameStr}] = value;
      break;
    }
    case OP_GET_LOCAL: {
      ZoneScopedNC("GET LOCAL", tracy::Color::Pink);
#ifdef BENCHMARK
      auto t1 = std::chrono::high_resolution_clock::now();
#endif
      uint8_t slot = frame->readByte();
      stack.push_back(frame->slots[slot]);
      FrameMark;
#ifdef BENCHMARK
      auto t2 = std::chrono::high_resolution_clock::now();
      /* Getting number of milliseconds as an integer. */
      auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
      times[OP_GET_LOCAL] += ms_int.count();
      calls[OP_GET_LOCAL]++;
#endif
      break;
    }
    case OP_SET_LOCAL: {
      ZoneScopedNC("SET LOCAL", tracy::Color::Pink);
      uint8_t slot = frame->readByte();
      frame->slots[slot] = stack.back();
      FrameMark;
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = frame->readByte();
      stack.push_back(*(frame->closure->upvalues[slot].location));
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = frame->readByte();
      *(frame->closure->upvalues[slot].location) = stack.back();
      break;
      break;
    }
    case OP_POP: {
      ZoneScopedNC("POP", tracy::Color::CadetBlue);
      stack.pop_back();
      FrameMark;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      ZoneScopedNC("JUMP", tracy::Color::Black);
      uint16_t offset = frame->readShort();
      if (isFalsey(stack.back())) {
        frame->ip() += offset;
      }
      FrameMark;
      break;
    }
    case OP_JUMP:
      frame->ip() += frame->readShort();
      break;
    case OP_LOOP: {
      uint16_t offset = frame->readShort();
      frame->ip() -= offset;
      break;
    }
    case OP_CALL: {
      ZoneScopedNC("call", tracy::Color::Orange);
      uint8_t argCount = frame->readByte();
      if (!callValue(stack[stack.size() - 1 - argCount], argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &frames.back();

      FrameMark;
      break;
    }
    case OP_CLOSURE: {
      ZoneScopedNC("closure", tracy::Color::Orange);
      // TODO: This ought to be refactored.
      // Need to be careful here, because we've mixed values, references,
      // pointers and smart pointers.
      const Function *funPtr =
          std::get<const Function *>(frame->readConstantRef());
      Closure closure{funPtr};
      for (int i = 0; i < closure.function->upvalueCount; i++) {
        uint8_t isLocal = frame->readByte();
        uint8_t index = frame->readByte();
        if (isLocal) {
          closure.upvalues.push_back(captureUpvalue(frame->slots + index));
        } else {
          closure.upvalues.push_back(frame->closure->upvalues[index]);
        }
      }
      closures.push_back(closure);
      stack.push_back(&closures.back());
      FrameMark;
      break;
    }
    }
  }
  FrameMark;
  return INTERPRET_COMPILE_ERROR;
};
