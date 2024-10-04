#include "function.hpp"
#include "chunk.hpp"
#include "value.hpp"

Function::Function(Chunk *chunk)
    : Obj(Type::FUNCTION), arity{0}, name{""}, upvalueCount{0}, chunk{chunk} {}
