#include "call_frame.hpp"
#include "function.hpp"

CallFrame::CallFrame(const Closure *closure, StackIterator slots)
    : closure{closure}, slots{slots},
      code{closure->function->chunk->code().begin()},
      constants{closure->function->chunk->getConstants().getValues()} {}
