#include "call_frame.hpp"
#include "chunk.hpp"
#include "function.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

CallFrame::CallFrame(const Closure *closure, StackIterator slots)
    : closure{closure}, slots{slots} {}
