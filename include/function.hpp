#pragma once

#include "boost/container/static_vector.hpp"
#include "chunk.hpp"
#include "function.fwd.hpp"
#include "value.hpp"
#include <_types/_uint8_t.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

using NativeFunctionPtr = Value (*)(int, Value *);
class NativeFunction {
public:
  NativeFunctionPtr fun;
};

class Function {
public:
  Function(Chunk *chunk);

  Chunk *chunk;
  size_t upvalueCount;
  std::string name;
  uint8_t arity;
};

// Not to be confused with Compiler "Upvalue".
class UpvalueObj {
public:
  Value *location;
};

class Closure {
public:
  explicit Closure(const Function *function) : function{function} {
    upvalues.reserve(function->upvalueCount);
  };
  const Function *function;
  std::vector<UpvalueObj> upvalues;

private:
};
