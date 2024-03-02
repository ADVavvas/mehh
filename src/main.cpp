#include "chunk.hpp"
#include "debug.hpp"
#include <iostream>

int main() {
  Chunk chunk{};
  chunk.write(OpCode::OP_RETURN);
  size_t constant = chunk.writeConstant(1.2);
  chunk.write(OP_CONSTANT);
  chunk.write(constant);
  disassembleChunk(chunk, "Test Chunk");

  return 0;
}
