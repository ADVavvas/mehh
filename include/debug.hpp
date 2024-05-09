#pragma once
#include "chunk.hpp"
#include <string_view>

void disassembleChunk(const Chunk &chunk, std::string_view name);

[[nodiscard]] size_t disassembleInstruction(const Chunk &chunk, size_t offset);

[[nodiscard]] size_t simpleInstruction(const std::string_view name,
                                       size_t offset);

[[nodiscard]] size_t constantInstruction(const std::string_view name,
                                         const Chunk &chunk, size_t offset);

[[nodiscard]] size_t byteInstruction(const std::string_view name,
                                     const Chunk &chunk, size_t offset);