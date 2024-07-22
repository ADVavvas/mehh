#include "function.hpp"
#include "chunk.hpp"

Function::Function(Chunk *chunk)
    : arity{0}, name{""}, upvalueCount{0}, chunk{chunk} {}
