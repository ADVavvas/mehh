#include "vm.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include <iostream>

inline const uint8_t VM::readByte() { return *ip++; }

inline const Value VM::readConstant() {
  return chunk->getConstants().getValues()[readByte()];
}

const InterpretResult VM::interpret(const Chunk &chunk) {
  this->chunk = &chunk;
  ip = chunk.getCode().begin();
  return run();
}

const InterpretResult VM::run() {
  for (;;) {
    uint8_t instruction;
    switch (instruction = readByte()) {
    case OP_RETURN:
      return INTERPRET_OK;
    case OP_CONSTANT:
      const Value &constant = readConstant();
      printValue(constant);
      std::cout << "\n";
      break;
    }
  }
};