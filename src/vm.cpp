#include "vm.hpp"
#include "Tracy.hpp"
#include "call_frame.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
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
#include <sys/cdefs.h>
#include <variant>
#include <vector>

// #define BENCHMARK

VM::VM() noexcept : compiler(Compiler{stringIntern}) {
  defineNative("clock", &native);
}

__attribute__((always_inline)) inline const bool
VM::isFalsey(const Value &val) {
  return std::holds_alternative<nil>(val) ||
         (std::holds_alternative<bool>(val) && !std::get<bool>(val));
}

__attribute__((always_inline)) inline const bool
VM::valuesEqual(const Value &a, const Value &b) {
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

__attribute__((always_inline)) inline const bool
VM::callValue(const Value &callee, const uint8_t argCount) {
  if (std::holds_alternative<const Closure *>(callee)) {
    bool res = call(std::get<const Closure *>(callee), argCount);
    return res;
  } else if (std::holds_alternative<const NativeFunction *>(callee)) {
    Value result = std::get<const NativeFunction *>(callee)->fun(
        argCount, stack.end().get_ptr() - argCount);
    stack.erase(stack.end() - argCount, stack.end());
    stack.push_back(result);
    return true;
  } else {
    runtimeError("Can only call functions and classes");
    return false;
  }
}

const UpvalueObj VM::captureUpvalue(const StackIterator &local) {
  return UpvalueObj{local.get_ptr()};
}

const bool VM::call(const Closure *closure, const uint8_t argCount) {
  if (__builtin_expect(argCount == closure->function->arity, 1)) {
    if (__builtin_expect(frames.size() < FRAME_MAX, 1)) {
      // (end - argCount - 1) accounts for the 0th slot
      frames.emplace_back(closure, stack.end() - argCount - 1);
      CallFrame frame{};
      return true;
    } else {
      runtimeError("Stack overflow");
      return false;
    }
  } else {
    runtimeError("Expected {} arguments but got {}.", closure->function->arity,
                 argCount);
    return false;
  }
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
  frame = &frames.back();
#ifdef BENCHMARK
  uint32_t numInstructions = 0;
  std::array<uint32_t, OP_RETURN + 1> calls{};
  calls.fill(0);
  std::array<uint64_t, OP_RETURN + 1> times{};
  times.fill(0);
  uint64_t loops = 0;
  uint64_t loop_time = 0;
#endif
  for (;;) {
#ifdef BENCHMARK
    auto l1 = std::chrono::high_resolution_clock::now();
    numInstructions++;
#endif
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
#ifdef BENCHMARK
    auto t1 = std::chrono::high_resolution_clock::now();
#endif
    uint8_t instruction = frame->readByte();

#ifdef BENCHMARK
    auto t2 = std::chrono::high_resolution_clock::now();
    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
    times[OP_NIL] += ms_int.count();
    calls[OP_NIL]++;
#endif
    switch (instruction) {
    case OP_RETURN: {
      auto res = op_return();
      if (res != INTERPRET_CONTINUE)
        return res;
      break;
    }
    case OP_CONSTANT: {
      op_constant();
      break;
    }
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
      auto res = op_add();
      if (__builtin_expect(res != INTERPRET_CONTINUE, false)) {
        return res;
      }
      break;
    }
    case OP_SUBTRACT: {
      auto res = op_subtract();
      if (__builtin_expect(res != INTERPRET_CONTINUE, false)) {
        return res;
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
      Value a = std::move(stack.back());
      stack.pop_back();
      Value b = std::move(stack.back());
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
      op_less();
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
      const Value &value = frame->readConstantRef();
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
#ifdef BENCHMARK
      auto t1 = std::chrono::high_resolution_clock::now();
#endif
      uint16_t offset = frame->readShort();
      if (isFalsey(stack.back())) {
        frame->ip() += offset;
      }
#ifdef BENCHMARK
      auto t2 = std::chrono::high_resolution_clock::now();
      /* Getting number of milliseconds as an integer. */
      auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
      times[OP_JUMP_IF_FALSE] += ms_int.count();
      calls[OP_JUMP_IF_FALSE]++;
#endif
      FrameMark;
      break;
    }
    case OP_JUMP: {
#ifdef BENCHMARK
      auto t1 = std::chrono::high_resolution_clock::now();
#endif
      frame->ip() += frame->readShort();
#ifdef BENCHMARK
      auto t2 = std::chrono::high_resolution_clock::now();
      /* Getting number of milliseconds as an integer. */
      auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
      times[OP_JUMP] += ms_int.count();
      calls[OP_JUMP]++;
#endif
      break;
    }
    case OP_LOOP: {
      uint16_t offset = frame->readShort();
      frame->ip() -= offset;
      break;
    }
    case OP_CALL: {
      auto res = op_call();
      if (__builtin_expect(res != INTERPRET_CONTINUE, false)) {
        return res;
      }
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

#ifdef BENCHMARK
    auto l2 = std::chrono::high_resolution_clock::now();
    /* Getting number of milliseconds as an integer. */
    auto dur = duration_cast<std::chrono::nanoseconds>(t2 - t1);
    loop_time += ms_int.count();
    loops++;
#endif
  }
  FrameMark;
  return INTERPRET_COMPILE_ERROR;
};

InterpretResult VM::op_return() {
  Value result = std::move(stack.back());
  stack.pop_back();
  frames.pop_back();
  if (frames.empty()) {
    stack.pop_back();
#ifdef BENCHMARK
    std::cout << "Instructions: " << numInstructions << '\n';
    std::cout << "Stack capacity: " << stack.capacity() << '\n';
    std::cout << "SUB: " << calls[OP_SUBTRACT] << '\n';
    std::cout << "Time SUB: " << times[OP_SUBTRACT] << '\n';
    std::cout << "ADD: " << calls[OP_ADD] << '\n';
    std::cout << "Time ADD: " << times[OP_ADD] << ' '
              << times[OP_ADD] / calls[OP_ADD] << "ns/call" << '\n';
    std::cout << "LESS: " << calls[OP_LESS] << '\n';
    std::cout << "Time LESS: " << times[OP_LESS] << ' '
              << times[OP_LESS] / calls[OP_LESS] << "ns/call" << '\n';
    std::cout << "GET GLOBAL: " << calls[OP_GET_GLOBAL] << '\n';
    std::cout << "Time GET GLOBAL: " << times[OP_GET_GLOBAL] << ' '
              << times[OP_GET_GLOBAL] / calls[OP_GET_GLOBAL] << "ns/call"
              << '\n';
    std::cout << "GET LOCAL: " << calls[OP_GET_LOCAL] << '\n';
    std::cout << "Time GET LOCAL: " << times[OP_GET_LOCAL] << ' '
              << times[OP_GET_LOCAL] / calls[OP_GET_LOCAL] << "ns/call" << '\n';
    std::cout << "JUMP: " << calls[OP_JUMP] << '\n';
    std::cout << "Time JUMP: " << times[OP_JUMP] << '\n';
    std::cout << "JUMP IF FALSE: " << calls[OP_JUMP_IF_FALSE] << '\n';
    std::cout << "Time JUMP IF FALSE: " << times[OP_JUMP_IF_FALSE] << ' '
              << times[OP_JUMP_IF_FALSE] / calls[OP_JUMP_IF_FALSE] << "ns/call"
              << '\n';
    std::cout << "CONSTANT: " << calls[OP_CONSTANT] << '\n';
    std::cout << "Time CONSTANT: " << times[OP_CONSTANT] << ' '
              << times[OP_CONSTANT] / calls[OP_CONSTANT] << "ns/call" << '\n';
    std::cout << "CALL: " << calls[OP_CALL] << '\n';
    std::cout << "Time CALL: " << times[OP_CALL] << ' '
              << times[OP_CALL] / calls[OP_CALL] << "ns/call\n";
    std::cout << "Fetch: " << calls[OP_NIL] << '\n';
    std::cout << "Time Fetch: " << times[OP_NIL] << ' '
              << times[OP_NIL] / calls[OP_NIL] << "ns/call";
    std::cout << "Loops: " << loops << '\n';
    std::cout << "Time Loop: " << loop_time << ' ' << loop_time / loops
              << "ns/call";
#endif
    return INTERPRET_OK;
  }
  // Erase all the called function's stack window.
  stack.erase(frame->slots, stack.end());
  stack.push_back(result);
  frame = &frames.back();
  return INTERPRET_CONTINUE;
}

void VM::op_constant() {
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  const Value constant = frame->readConstant();
  stack.push_back(constant);
  // printValue(constant);
  // std::cout << "\n";
#ifdef BENCHMARK
  auto t2 = std::chrono::high_resolution_clock::now();
  /* Getting number of milliseconds as an integer. */
  auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
  times[OP_CONSTANT] += ms_int.count();
  calls[OP_CONSTANT]++;
#endif
}

InterpretResult VM::op_call() {
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  uint8_t argCount = frame->readByte();
  if (callValue(stack[stack.size() - 1 - argCount], argCount)) {
    frame = &frames.back();
#ifdef BENCHMARK
    auto t2 = std::chrono::high_resolution_clock::now();
    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
    times[OP_CALL] += ms_int.count();
    calls[OP_CALL]++;
#endif
    return INTERPRET_CONTINUE;
  }
  return INTERPRET_RUNTIME_ERROR;
}

void VM::op_less() {
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  // auto res = binary_op([](double a, double b) constexpr { return a < b;
  // });
  if (__builtin_expect(stack.size() >= 2, true)) {
    if (__builtin_expect(std::holds_alternative<double>(*(stack.end() - 1)) &&
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
  // if (res != INTERPRET_RUNTIME_ERROR) {
  //   break;
  // }
  // return res;
}

InterpretResult VM::op_add() {
  ZoneScopedNC("ADD", tracy::Color::Brown);
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
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
#ifdef BENCHMARK
    auto t2 = std::chrono::high_resolution_clock::now();
    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
    times[OP_ADD] += ms_int.count();
    calls[OP_ADD]++;
#endif
    if (__builtin_expect(success, true)) {
      return INTERPRET_CONTINUE;
    }
  }
  return INTERPRET_RUNTIME_ERROR;
}

InterpretResult VM::op_subtract() {
  ZoneScopedNC("SUB", tracy::Color::Crimson);
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  if (__builtin_expect(stack.size() >= 2 &&
                           std::holds_alternative<double>(*(stack.end() - 1)) &&
                           std::holds_alternative<double>(*(stack.end() - 2)),
                       true)) {

    double b = std::get<double>(stack.back());
    stack.pop_back();
    double &a = std::get<double>(stack.back());
    a -= b;
    FrameMark;
#ifdef BENCHMARK
    auto t2 = std::chrono::high_resolution_clock::now();
    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
    times[OP_SUBTRACT] += ms_int.count();
    calls[OP_SUBTRACT]++;
#endif
    return INTERPRET_CONTINUE;
  } else {
    runtimeError("Operands must be numbers");
    return INTERPRET_RUNTIME_ERROR;
  }
}
