#include "chunk.hpp"
#include <cstddef>
#include <cstdint>

void Chunk::write(uint8_t byte, size_t line) noexcept {
  code.push_back(byte);
  if (lines.empty()) {
    lines.push_back(Line{1, line});
  } else {
    Line &l = lines.back();
    if (l.line == line) {
      l.count += 1;
    } else {
      lines.push_back(Line{1, line});
    }
  }
}

size_t Chunk::writeConstant(const Value &value) noexcept {
  return constants.write(value);
}

const std::vector<uint8_t> &Chunk::getCode() const noexcept { return code; }

// TODO: Debug
const std::vector<Line> &Chunk::getLines() const noexcept { return lines; }

const uint8_t Chunk::getLine(size_t offset) const noexcept {
  int i = 0;
  int count = 0;
  if (lines.empty()) {
    return 0;
  }

  if (offset == 0) {
    return lines[0].line;
  }

  while (count <= offset) {
    count += lines[i++].count;
  }
  return lines[--i].line;
}

const ValueArray &Chunk::getConstants() const noexcept { return constants; }

const size_t Chunk::count() const noexcept { return code.size(); }