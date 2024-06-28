#pragma once

#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

class ValueArray {
public:
  size_t write(const Value &value) noexcept;
  [[nodiscard]] const std::vector<Value> &getValues() const noexcept;
  [[nodiscard]] const size_t count() const noexcept;

private:
  uint32_t capacity;
  std::vector<Value> values;
};

void printValue(const Value &value);
