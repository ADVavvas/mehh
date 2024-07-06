#include "call_frame.hpp"
#include "chunk.hpp"
#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

CallFrame::CallFrame(Function function, std::vector<Value>::iterator slots)
    : function{function}, slots{slots} {};

size_t &CallFrame::ip() { return ip_; }

const size_t CallFrame::getIp() const { return ip_; }

const uint8_t CallFrame::readByte() {
  return function.chunk.getCode()[ip_++];
}

const uint16_t CallFrame::readShort() {
  ip_ += 2;
  return function.chunk.getCode()[ip_ - 1] |
         (function.chunk.getCode()[ip_ - 2] << 8);
}

const Value CallFrame::readConstant() {
  return function.chunk.getConstants().getValues()[readByte()];
}
