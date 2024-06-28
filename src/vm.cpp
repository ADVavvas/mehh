#include "vm.hpp"
#include "box.hpp"
#include "call_frame.hpp"
#include "chunk.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "function.hpp"
#include "value.hpp"
#include "value_array.hpp"
#include <_types/_uint8_t.h>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

VM::VM() noexcept
    : compiler(Compiler{stringIntern, FunctionType::TYPE_SCRIPT}) {
  stack.reserve(STACK_MAX);
}

inline const bool VM::isFalsey(const Value &val) {
  return std::holds_alternative<nil>(val) ||
         (std::holds_alternative<bool>(val) && std::get<bool>(val));
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

const InterpretResult VM::interpret(const std::string_view source) {
  const std::optional<box<Function>> &function = compiler.compile(source);
  if (!function.has_value()) {
    return INTERPRET_COMPILE_ERROR;
  }
  stack.push_back(function.value());
  frames.push_back(CallFrame{*function.value(), stack.begin()});
  return run();
}

const InterpretResult VM::run() {
  CallFrame &frame = frames.back();
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (auto val : stack) {
      std::cout << "[ ";
      printValue(val);
      std::cout << " ]";
    }
    std::cout << "\n";
    disassembleInstruction(frame.function.chunk, frame.getIp());
#endif
    uint8_t instruction;
    switch (instruction = frame.readByte()) {
    case OP_RETURN:
      return INTERPRET_OK;
    case OP_CONSTANT: {
      const Value &constant = frame.readConstant();
      stack.push_back(constant);
      // printValue(constant);
      // std::cout << "\n";
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
      if (stack.size() < 2) {
        return INTERPRET_RUNTIME_ERROR;
      }

      Value b = stack.back();
      stack.pop_back();
      Value a = stack.back();
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
      if (success) {
        break;
      }
      return INTERPRET_RUNTIME_ERROR;
    }
    case OP_SUBTRACT: {
      auto res = binary_op([](double a, double b) constexpr { return a - b; });
      if (res == INTERPRET_RUNTIME_ERROR) {

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
      auto res = binary_op([](double a, double b) constexpr { return a < b; });
      if (res == INTERPRET_RUNTIME_ERROR) {
        return res;
      }
      break;
    }
    case OP_PRINT: {
      printValue(stack.back());
      std::cout << "\n";
      stack.pop_back();
      break;
    }
    case OP_DEFINE_GLOBAL: {
      const Value &name = frame.readConstant();
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
      const Value &value = frame.readConstant();
      if (!std::holds_alternative<std::string_view>(value)) {
        break;
      }
      const auto &variable = globals.find(std::get<std::string_view>(value));
      if (variable == globals.end()) {
        runtimeError("Undefined variable '{}.'",
                     std::get<std::string_view>(value));
        return INTERPRET_RUNTIME_ERROR;
      }
      stack.push_back(variable->second);
      break;
    }
    case OP_SET_GLOBAL: {
      const Value &name = frame.readConstant();
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
      uint8_t slot = frame.readByte();
      stack.push_back(frame.slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = frame.readByte();
      frame.slots[slot] = stack.back();
      break;
    }
    case OP_POP:
      stack.pop_back();
      break;
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = frame.readShort();
      if (isFalsey(stack.back())) {
        frame.ip() += offset;
      }
      break;
    }
    case OP_JUMP:
      frame.ip() += frame.readShort();
      break;
    case OP_LOOP: {
      uint16_t offset = frame.readShort();
      frame.ip() -= offset;
      break;
    }
    }
  }
  return INTERPRET_COMPILE_ERROR;
};
