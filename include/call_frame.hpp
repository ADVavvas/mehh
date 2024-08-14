#pragma once

#include "boost/container/static_vector.hpp"
#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

using StackIterator = boost::container::static_vector<Value, 256>::iterator;

class CallFrame {
public:
  CallFrame(const Closure *closure, StackIterator slots);

  [[nodiscard]] size_t &ip();
  [[nodiscard]] const size_t getIp() const;
  [[nodiscard]] const uint8_t readByte();
  [[nodiscard]] const uint16_t readShort();
  [[nodiscard]] const Value readConstant();
  [[nodiscard]] const Value &readConstantRef();

  const Closure *closure;
  StackIterator slots;

private:
  size_t ip_ = 0;
};
