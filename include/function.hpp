#pragma once

#include "chunk.hpp"
#include "function.fwd.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class NativeFunction {
public:
  std::function<Value(int, std::vector<Value>::iterator)> fun;
};

class Function {
public:
  Function();

  size_t upvalueCount;
  uint8_t arity;
  Chunk chunk;
  std::string name;
};

// Not to be confused with Compiler "Upvalue".
class UpvalueObj {
public:
  std::vector<Value>::iterator location;
};

class Closure {
public:
  Closure(const Function *function) : function{function} {
    upvalues.reserve(function->upvalueCount);
  };
  const Function *function;
  std::vector<UpvalueObj> upvalues;

private:
};
