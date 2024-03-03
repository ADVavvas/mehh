#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp"
#include <iostream>

int main() {
  VM vm{};
  Chunk chunk{};
  size_t constant1 = chunk.writeConstant(1.2);
  size_t constant2 = chunk.writeConstant(42);
  size_t constant3 = chunk.writeConstant(199);
  chunk.write(OP_CONSTANT, 123);
  chunk.write(constant1, 123);
  chunk.write(OpCode::OP_RETURN, 123);
  chunk.write(OP_CONSTANT, 124);
  chunk.write(constant2, 124);
  chunk.write(OP_CONSTANT, 124);
  chunk.write(constant2, 124);
  chunk.write(OP_CONSTANT, 125);
  chunk.write(constant3, 125);
  chunk.write(OpCode::OP_RETURN, 126);
  disassembleChunk(chunk, "Test Chunk");
  InterpretResult result = vm.interpret(chunk);
  vm.run();
  return 0;
}
