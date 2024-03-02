#include "chunk.hpp"
#include "debug.hpp"
#include <iostream>

int main() {
  Chunk chunk{};
  size_t constant = chunk.writeConstant(1.2);
  chunk.write(OP_CONSTANT, 123);
  chunk.write(constant, 123);
  chunk.write(OpCode::OP_RETURN, 123);
  disassembleChunk(chunk, "Test Chunk");

  return 0;
}
