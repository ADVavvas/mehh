#include "value.hpp"
#include "box.hpp"
#include "function.hpp"
#include <iostream>
#include <string_view>
#include <variant>

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
                 [](const box<Function> function) {
                   if (function->name.empty()) {
                     std::cout << "<script>";
                     return;
                   }
                   std::cout << "<fn " << function->name << ">";
                 },
                 [](const box<NativeFunction> function) {
                   std::cout <<"<native fn>";
                }
             },
             value);
}
