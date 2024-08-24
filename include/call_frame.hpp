#pragma once

#include "boost/container/static_vector.hpp"
#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

using StackIterator = boost::container::static_vector<Value, 64>::iterator;

class CallFrame {
public:
  CallFrame(const Closure *const closure, StackIterator slots);

  // [[nodiscard]] size_t &ip();
  // [[nodiscard]] const size_t getIp() const;
  [[nodiscard]] std::vector<uint8_t>::iterator &ip();
  [[nodiscard]] const std::vector<uint8_t>::const_iterator getIp() const;
  [[nodiscard]] const uint8_t readByte();
  [[nodiscard]] const uint16_t readShort();
  [[nodiscard]] const Value readConstant();
  [[nodiscard]] const Value &readConstantRef();

  const Closure *const closure;
  StackIterator slots;

private:
  std::vector<uint8_t>::iterator code;
  size_t ip_ = 0;
};

// __attribute__((always_inline)) inline size_t &CallFrame::ip() { return ip_; }

__attribute__((always_inline)) inline std::vector<uint8_t>::iterator &CallFrame::ip() {
  return code;
}

// __attribute__((always_inline)) inline const size_t CallFrame::getIp() const {
//   return ip_;
// }

__attribute__((always_inline)) inline const std::vector<uint8_t>::const_iterator CallFrame::getIp() const {
  return code;
}

// __attribute__((always_inline)) inline const uint8_t CallFrame::readByte() {
//   return closure->function->chunk->getCode()[ip_++];
// }

__attribute__((always_inline)) inline const uint8_t CallFrame::readByte() {
  return *code++;
}

// __attribute__((always_inline)) inline const uint16_t CallFrame::readShort() {
//   ip_ += 2;
//   return closure->function->chunk->getCode()[ip_ - 1] |
//          (closure->function->chunk->getCode()[ip_ - 2] << 8);
// }

__attribute__((always_inline)) inline const uint16_t CallFrame::readShort() {
  code += 2;
  return *(code-1) |
         (*(code - 2) << 8);
}

__attribute__((always_inline)) inline const Value CallFrame::readConstant() {
  return closure->function->chunk->getConstants().getValues()[readByte()];
}
__attribute__((always_inline)) inline const Value &
CallFrame::readConstantRef() {
  return closure->function->chunk->getConstants().getValues()[readByte()];
}
