#include "chunk.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

void Chunk::write(uint8_t byte, size_t line) noexcept {
  code_.push_back(byte);
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

const size_t Chunk::count() const noexcept { return code_.size(); }
