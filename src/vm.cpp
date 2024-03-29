#include "vm.hpp"
#include "chunk.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include <iostream>
#include <string_view>
#include <variant>

VM::VM() noexcept { stack.reserve(STACK_MAX); }

inline const uint8_t VM::readByte() { return *ip++; }

inline const Value VM::readConstant() {
  return chunk->getConstants().getValues()[readByte()];
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
          [](const auto a, const auto b) { return false; },
      },
      a, b);
}

const InterpretResult VM::interpret(const std::string_view source) {
  const std::optional<Chunk> &chunk = compiler.compile(source);
  if (!chunk.has_value())
    return INTERPRET_COMPILE_ERROR;
  this->chunk = &chunk.value();
  ip = this->chunk->getCode().begin();
  return run();
}

const InterpretResult VM::run() {
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    std::cout << "          ";
    for (auto val : stack) {
      std::cout << "[ ";
      printValue(val);
      std::cout << " ]";
    }
    std::cout << "\n";
    disassembleInstruction(*chunk, ip - chunk->getCode().begin());
#endif
    uint8_t instruction;
    switch (instruction = readByte()) {
    case OP_RETURN:
      printValue(stack.back());
      std::cout << "\n";
      stack.pop_back();
      return INTERPRET_OK;
    case OP_CONSTANT: {
      const Value &constant = readConstant();
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
      auto res = binary_op([](double a, double b) constexpr { return a + b; });
      if (res == INTERPRET_RUNTIME_ERROR) {

        return res;
      }
      break;
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
    }
  }
  return INTERPRET_COMPILE_ERROR;
};