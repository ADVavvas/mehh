#pragma once

#include <cstdint>
#include <vector>
using Value = double;

class ValueArray {
public:
  size_t write(Value value) noexcept;
  [[nodiscard]] const std::vector<Value> &getValues() const noexcept;
  [[nodiscard]] const size_t count() const noexcept;

private:
  uint32_t capacity;
  std::vector<Value> values;
};