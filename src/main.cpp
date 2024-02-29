#include "chunk.hpp"
#include "debug.hpp"
#include <iostream>

int main() {
  Chunk chunk{};
  chunk.write(OpCode::OP_RETURN);
  disassembleChunk(chunk, "Test Chunk");

  return 0;
}
