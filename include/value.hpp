#pragma once

#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

using nil = std::monostate;
using Value = std::variant<double, bool, nil, std::string_view>;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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