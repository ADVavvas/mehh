#include "debug.hpp"
#include "chunk.hpp"
#include <cstdint>
#include <fmt/core.h>
#include <format>
#include <iomanip>
#include <iostream>

void disassembleChunk(const Chunk &chunk, const std::string_view name) {
  std::cout << fmt::format("== {} ==\n", name);

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
  case OP_PRINT:
    return simpleInstruction("OP_PRINT", offset);
  case OP_POP:
    return simpleInstruction("OP_POP", offset);
  case OP_DEFINE_GLOBAL:
    return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_GET_GLOBAL:
    return constantInstruction("OP_GET_GLOBAL", chunk, offset);
  case OP_SET_GLOBAL:
    return constantInstruction("OP_SET_GLOBAL", chunk, offset);
  case OP_GET_LOCAL:
    return byteInstruction("OP_GET_LOCAL", chunk, offset);
  case OP_SET_LOCAL:
    return byteInstruction("OP_SET_LOCAL", chunk, offset);
  case OP_JUMP:
    return byteInstruction("OP_JUMP", chunk, offset);
  case OP_JUMP_IF_FALSE:
    return byteInstruction("OP_JUMP_IF_FALSE", chunk, offset);
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

size_t byteInstruction(const std::string_view name, const Chunk &chunk,
                       size_t offset) {
  uint8_t slot = chunk.getCode()[offset + 1];
  std::cout << name << " " << slot << '\'';
  return offset + 2;
}

size_t jumpInstruction(const std::string_view name, const int sign,
                       const Chunk &chunk, size_t offset) {
  uint16_t jump =
      chunk.getCode()[offset + 1] << 8 | chunk.getCode()[offset + 2];
  std::cout << name << " " << offset << " -> " << offset + 3 + sign * jump
            << "\n";
  return offset + 3;
}