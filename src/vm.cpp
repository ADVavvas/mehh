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
    case OP_NEGATE: {
      Value &val = stack.back();
      if (!std::holds_alternative<double>(val)) {
        runtimeError("Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }
      val = -std::get<double>(val);
    } break;
    case OP_ADD:
      binary_op([](Value a, Value b) constexpr { return a + b; });
      break;
    case OP_SUBTRACT:
      binary_op([](Value a, Value b) constexpr { return a - b; });
      break;
    case OP_MULTIPLY:
      binary_op([](Value a, Value b) constexpr { return a * b; });
      break;
    case OP_DIVIDE:
      binary_op([](Value a, Value b) constexpr { return a / b; });
      break;
    }
  }
  return INTERPRET_COMPILE_ERROR;
};