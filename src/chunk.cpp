#include "chunk.hpp"
#include <cstdint>

void Chunk::write(uint8_t byte) noexcept { code.push_back(byte); }

size_t Chunk::writeConstant(Value &&value) noexcept {
  return constants.write(value);
}

const std::vector<uint8_t> &Chunk::getCode() const noexcept { return code; }

const ValueArray &Chunk::getConstants() const noexcept { return constants; }

const size_t Chunk::count() const noexcept { return code.size(); }