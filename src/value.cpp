#include "value.hpp"
#include "function.hpp"
#include <iostream>

void printValue(const Value &value) {
  switch (value.getType()) {
  case ValueType::NIL:
    std::cout << "nil";
    break;
  case ValueType::NUMBER:
    std::cout << value.asNumber();
    break;
  case ValueType::BOOL: {
    auto val = value.asBool();
    if (val) {
      std::cout << "true";
    } else {
      std::cout << "false";
    }
  } break;
  case ValueType::STRING:
    std::cout << value.asObj()->as<StringObj>()->str << '\n';
    break;
  case ValueType::FUNCTION: {
    auto function = value.asObj()->as<Function>();
    if (function->name.empty()) {
      std::cout << "<script>";
      return;
    }
    std::cout << "<fn " << function->name << ">";
  } break;
  case ValueType::NATIVE_FUNCTION:
    std::cout << "<native fn>";
    break;
  case ValueType::CLOSURE: {
    auto val = Value{value.asObj()->as<Closure>()->function};
    printValue(val);
    break;
  }
  case ValueType::UPVALUE:
    std::cout << "upvalue";
    break;
  case ValueType::OBJ:
    std::cout << "obj";
    break;
  }
}
