#include "debug.hpp"
#include "chunk.hpp"
#include <cstdint>
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

  const OpCode &instruction{chunk.getCode()[offset]};
  switch (instruction) {
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  default:
    std::cout << "Unknown opcode: " << instruction;
  }
}

size_t simpleInstruction(std::string_view name, size_t offset) {
  std::cout << name << "\n";
  return offset + 1;
}
size_t constantInstruction(std::string_view name, Chunk &chunk, size_t offset) {
  uint8_t constant = chunk.getCode()[offset + 1];
  std::cout << name << " " << constant << '\''
            << chunk.getConstants().getValues()[constant] << '\'' << "\n";
  return offset + 2;
}