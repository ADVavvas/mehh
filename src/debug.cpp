#include "debug.hpp"
#include "chunk.hpp"
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>

void disassembleChunk(const Chunk &chunk, const std::string_view name) {
  std::cout << std::format("== {} ==\n", name);

  for (size_t offset = 0; offset < chunk.count();) {
    offset = disassembleInstruction(chunk, offset);
  }
}

size_t disassembleInstruction(const Chunk &chunk, size_t offset) {
  std::cout << std::setfill('0') << std::right << std::setw(4) << offset << ' ';

  if (offset >= chunk.count())
    return chunk.count() - 1;

  uint8_t line = chunk.getLine(offset);
  if (offset > 0 && line == chunk.getLine(offset - 1)) {
    std::cout << "    | ";
  } else {
    std::cout << static_cast<int>(line) << ' ';
  }

  const OpCode &instruction{chunk.getCode()[offset]};
  switch (instruction) {
  case OP_NIL:
    return simpleInstruction("OP_NIL", offset);
  case OP_TRUE:
    return simpleInstruction("OP_TRUE", offset);
  case OP_FALSE:
    return simpleInstruction("OP_FALSE", offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_NOT:
    return simpleInstruction("OP_NOT", offset);
  case OP_EQUAL:
    return simpleInstruction("OP_EQUAL", offset);
  case OP_GREATER:
    return simpleInstruction("OP_GREATER", offset);
  case OP_LESS:
    return simpleInstruction("OP_LESS", offset);
  default:
    std::cout << "Unknown opcode: " << instruction;
    return offset;
  }
}

size_t simpleInstruction(const std::string_view name, size_t offset) {
  std::cout << name << "\n";
  return offset + 1;
}
size_t constantInstruction(const std::string_view name, const Chunk &chunk,
                           size_t offset) {
  uint8_t constant = chunk.getCode()[offset + 1];
  std::cout << name << " " << constant << '\'';
  printValue(chunk.getConstants().getValues()[constant]);
  std::cout << '\'' << "\n";
  return offset + 2;
}
