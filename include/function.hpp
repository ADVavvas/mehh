#pragma once

#include "chunk.hpp"
#include <cstdint>
#include <string>

class Function {
public:
  Function();

  uint8_t arity;
  Chunk chunk;
  std::string name;
};
