#pragma once

#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

class CallFrame {
public:
  CallFrame(Closure closure, std::vector<Value>::iterator slots);

  [[nodiscard]] size_t &ip();
  [[nodiscard]] const size_t getIp() const;
  [[nodiscard]] const uint8_t readByte();
  [[nodiscard]] const uint16_t readShort();
  [[nodiscard]] const Value readConstant();
  [[nodiscard]] const Value &readConstantRef();

  Closure closure;
  std::vector<Value>::iterator slots;

private:
  size_t ip_ = 0;
};
