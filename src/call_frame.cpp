#include "call_frame.hpp"
#include "chunk.hpp"
#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

CallFrame::CallFrame(Closure closure, std::vector<Value>::iterator slots)
    : closure{closure}, slots{slots} {};

size_t &CallFrame::ip() { return ip_; }

const size_t CallFrame::getIp() const { return ip_; }

const uint8_t CallFrame::readByte() {
  return closure.function->chunk.getCode()[ip_++];
}

const uint16_t CallFrame::readShort() {
  ip_ += 2;
  return closure.function->chunk.getCode()[ip_ - 1] |
         (closure.function->chunk.getCode()[ip_ - 2] << 8);
}

const Value CallFrame::readConstant() {
  return closure.function->chunk.getConstants().getValues()[readByte()];
}
const Value &CallFrame::readConstantRef() {
  return closure.function->chunk.getConstants().getValues()[readByte()];
}
