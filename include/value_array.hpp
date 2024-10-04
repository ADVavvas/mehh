#pragma once

#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

// TODO: This is literally just a vector - get rid of ValueArray
class ValueArray {
public:
  __attribute__((always_inline)) inline size_t write(const Value &value) noexcept;
  [[nodiscard]] __attribute__((always_inline)) inline const std::vector<Value> &getValues() const noexcept;
  [[nodiscard]] __attribute__((always_inline)) inline const size_t count() const noexcept;

private:
  uint32_t capacity;
  std::vector<Value> values;
};

size_t ValueArray::write(const Value &value) noexcept {
  values.push_back(value);
  return values.size() - 1;
}

[[nodiscard]] const std::vector<Value> &ValueArray::getValues() const noexcept {
  return values;
}

[[nodiscard]] const size_t ValueArray::count() const noexcept {
  return values.size();
}

void printValue(const Value &value);

