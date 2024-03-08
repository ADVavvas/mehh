#pragma once
#include "token.hpp"

struct Parser {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
};