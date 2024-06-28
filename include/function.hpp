#pragma once

#include "chunk.hpp"
#include <cstdint>
#include <string_view>

class Function {
public:
  Function();

  uint8_t arity;
  Chunk chunk;
  std::string_view name;
};
