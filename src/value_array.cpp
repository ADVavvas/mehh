#include "value_array.hpp"
#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <vector>

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
