#include "chunk.hpp"

void Chunk::write(OpCode opcode) noexcept { code.push_back(opcode); }

const std::vector<OpCode> &Chunk::codes() const noexcept { return code; }

const size_t Chunk::count() const noexcept { return code.size(); }