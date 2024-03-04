#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp"
#include <iostream>

int main() {
  VM vm{};
  Chunk chunk{};
  size_t constant = chunk.writeConstant(1.2);
  chunk.write(OP_CONSTANT, 123);
  chunk.write(constant, 123);

  size_t constant2 = chunk.writeConstant(3.4);
  chunk.write(OP_CONSTANT, 123);
  chunk.write(constant2, 123);

  chunk.write(OP_ADD, 123);

  size_t constant3 = chunk.writeConstant(5.6);
  chunk.write(OP_CONSTANT, 123);
  chunk.write(constant3, 123);

  chunk.write(OP_DIVIDE, 123);
  chunk.write(OP_NEGATE, 123);

  chunk.write(OP_RETURN, 123);
  disassembleChunk(chunk, "Test Chunk");
  InterpretResult result = vm.interpret(chunk);
  return 0;
}
