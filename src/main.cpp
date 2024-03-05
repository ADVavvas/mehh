#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp"
#include <iostream>

#include "Tracy.hpp"

constexpr int RUNS = 1'000'000;

void profileNegation() {
  ZoneScoped;
  VM vm{};
  Chunk chunk{};
  size_t constant = chunk.writeConstant(0.001);
  chunk.write(OP_CONSTANT, 1);
  chunk.write(constant, 1);
  for (int i = 0; i < RUNS; ++i) {
    chunk.write(OP_NEGATE, 1);
  }

  chunk.write(OP_RETURN, 2);
  InterpretResult result = vm.interpret(chunk);
}

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
