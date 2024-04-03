#include "value.hpp"
#include <iostream>
#include <variant>

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

void printValue(const Value &value) {

  std::visit(overloaded{
                 [](const std::monostate val) { std::cout << "nil"; },
                 [](const bool val) {
                   if (val) {
                     std::cout << "true";
                   } else {
                     std::cout << "false";
                   }
                 },
                 [](const double val) { std::cout << val; },
                 [](const std::string_view val) { std::cout << val; },
             },
             value);
}