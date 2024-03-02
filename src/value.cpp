#include "value.hpp"

size_t ValueArray::write(Value value) noexcept {
  values.push_back(value);
  return values.size() - 1;
}

[[nodiscard]] const std::vector<Value> &ValueArray::getValues() const noexcept {
  return values;
}

[[nodiscard]] const size_t ValueArray::count() const noexcept {
  return values.size();
}