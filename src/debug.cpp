#include "debug.hpp"
#include "chunk.hpp"
#include <format>
#include <iomanip>
#include <iostream>

void disassembleChunk(Chunk &chunk, std::string_view name) {
  std::cout << std::format("== {} ==\n", name);

  for (size_t offset = 0; offset < chunk.count();) {
    offset = disassembleInstruction(chunk, offset);
  }
}

size_t disassembleInstruction(Chunk &chunk, size_t offset) {
  std::cout << std::setfill('0') << std::right << std::setw(4) << offset << ' ';

  if (offset >= chunk.count())
    return chunk.count() - 1;

  const OpCode &instruction{chunk.codes()[offset]};
  switch (instruction) {
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  default:
    std::cout << "Unknown opcode: " << instruction;
  }
}

size_t simpleInstruction(std::string_view name, size_t offset) {
  std::cout << name << "\n";
  return offset + 1;
}