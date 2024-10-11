#pragma once

#include "chunk.hpp"
#include "function.fwd.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using NativeFunctionPtr = Value (*)(int, Value *);
class NativeFunction : public Obj {
public:
  NativeFunction(NativeFunctionPtr fun)
      : Obj{ValueType::NATIVE_FUNCTION}, fun{fun} {};
  NativeFunctionPtr fun;
};

class Function : public Obj {
public:
  explicit Function(Chunk *chunk)
      : Obj(ValueType::FUNCTION), arity{0}, name{""}, upvalueCount{0},
        chunk{chunk} {}

  Chunk *chunk;
  size_t upvalueCount;
  std::string name;
  uint8_t arity;
};

// Not to be confused with Compiler "Upvalue".
class UpvalueObj : public Obj {
public:
  explicit UpvalueObj(Value * const function) : Obj(ValueType::UPVALUE), location{function} {}
  Value * const location;
};

class Closure : public Obj {
public:
  explicit Closure(const Function *function)
      : Obj{ValueType::CLOSURE}, function{function} {
    upvalues.reserve(function->upvalueCount);
  };
  const Function *function;
  std::vector<UpvalueObj> upvalues;

private:
};
