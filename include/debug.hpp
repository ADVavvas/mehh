#pragma once
#include "chunk.hpp"
#include <string_view>

void disassembleChunk(Chunk &chunk, std::string_view name);

[[nodiscard]] size_t disassembleInstruction(Chunk &chunk, size_t offset);

[[nodiscard]] size_t simpleInstruction(std::string_view name, size_t offset);

[[nodiscard]] size_t constantInstruction(std::string_view name, Chunk &chunk,
                                         size_t offset);