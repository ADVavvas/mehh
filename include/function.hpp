#pragma once

#include "chunk.hpp"
#include "function.fwd.hpp"
#include "value.hpp"
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

  uint8_t arity;
  Chunk chunk;
  std::string name;
};
