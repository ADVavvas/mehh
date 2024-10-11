#include "vm.hpp"
#include "Tracy.hpp"
#include "call_frame.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "function.hpp"
#include "value.hpp"
#include "value_array.hpp"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdint.h>
#include <string>
#include <string_view>
#include <sys/cdefs.h>
#include <variant>
#include <vector>

// #define BENCHMARK
#define UNLIKELY(x) __builtin_expect(x, 0)
#define MUSTTAIL __attribute__((musttail))

VM::VM() noexcept : compiler(Compiler{stringIntern}) {
  defineNative("clock", &native);
}

__attribute__((always_inline)) inline const bool
VM::isFalsey(const Value &val) {
  return val.isNil() || (val.isBool() && !val.asBool());
}

__attribute__((always_inline)) inline const bool
VM::valuesEqual(const Value &a, const Value &b) {
  const auto typeA = a.getType();
  const auto typeB = b.getType();

  // TODO: Remove branches when possible
  if (typeA == ValueType::NIL && typeB == ValueType::NIL) {
    return true;
  } else if (typeA == ValueType::BOOL && typeB == ValueType::BOOL) {
    return a.asBool() == b.asBool();
  } else if (typeA == ValueType::NUMBER && typeB == ValueType::NUMBER) {
    return a.asNumber() == a.asNumber();
  } else if (typeA == ValueType::STRING && typeB == ValueType::STRING) {
    // TODO: Implement properly
    return a.asObj()->as<StringObj>()->str == b.asObj()->as<StringObj>()->str;
  } else {
    return false;
  }
}

__attribute__((always_inline)) inline const bool
VM::callValue(const Value &callee, const uint8_t argCount) {
  if (UNLIKELY(!callee.isObj())) {
    runtimeError("Can only call functions and classes");
    return false;
  }

  auto ptr = callee.asObj();

  switch (ptr->getType()) {
  case ValueType::CLOSURE: {

    bool res = call(static_cast<const Closure *>(ptr), argCount);
    return res;
  }
  case ValueType::NATIVE_FUNCTION: {
    Value result = static_cast<const NativeFunction *>(ptr)->fun(
        argCount, stack.end().get_ptr() - argCount);
    stack.erase(stack.end() - argCount, stack.end());
    stack.push_back(result);
    return true;
  }
  default: {
    runtimeError("Can only call functions and classes");
    return false;
  }
  }
}

__attribute__((always_inline)) const UpvalueObj
VM::captureUpvalue(const StackIterator &local) {
  return UpvalueObj{local.get_ptr()};
}

__attribute__((always_inline)) const bool VM::call(const Closure *closure,
                                                   const uint8_t argCount) {
  if (__builtin_expect(argCount == closure->function->arity, 1)) {
    if (__builtin_expect(frames.size() < FRAME_MAX, 1)) {
      // (end - argCount - 1) accounts for the 0th slot
      frames.emplace_back(closure, stack.end() - argCount - 1);
      return true;
    } else {
      runtimeError("Stack overflow");
      return false;
    }
  } else {
    runtimeError("Expected {} arguments but got {}.", closure->function->arity,
                 argCount);
    return false;
  }
}

void VM::defineNative(std::string name, NativeFunction *fn) {
  globals.insert_or_assign(stringIntern.intern(name)->str, Value{fn});
}

const InterpretResult VM::interpret(const std::string_view source) {
  const std::optional<const Function *> &function = compiler.compile(source);
  if (!function.has_value()) {
    return INTERPRET_COMPILE_ERROR;
  }
  const Function *fun = function.value();
  stack.push_back(Value{static_cast<const Obj *>(fun)});
  // TODO: This ought to be refactored.
  // Need to be careful here, because we've mixed values, references, pointers
  // and smart pointers.
  const Function *funPtr = stack.back().asObj()->as<const Function>();
  Closure closure{funPtr};
  if (!call(&closure, 0)) {
    return InterpretResult::INTERPRET_RUNTIME_ERROR;
  }
  return run();
}

InterpretResult VM::dispatch() {
  frame = &frames.back();

  uint8_t instruction = frame->readByte();

  switch (instruction) {
  case OP_RETURN:
    MUSTTAIL return op_return();
  case OP_CONSTANT:
    MUSTTAIL return op_constant();
  case OP_NIL:
    MUSTTAIL return op_nil();
  case OP_TRUE:
    MUSTTAIL return op_true();
  case OP_FALSE:
    MUSTTAIL return op_false();
  case OP_NEGATE:
    MUSTTAIL return op_negate();
  case OP_ADD:
    MUSTTAIL return op_add();
  case OP_SUBTRACT:
    MUSTTAIL return op_subtract();
  case OP_MULTIPLY:
    MUSTTAIL return op_multiply();
  case OP_DIVIDE:
    MUSTTAIL return op_divide();
  case OP_NOT:
    MUSTTAIL return op_not();
  case OP_EQUAL:
    MUSTTAIL return op_equal();
  case OP_GREATER:
    MUSTTAIL return op_greater();
  case OP_LESS:
    MUSTTAIL return op_less();
  case OP_PRINT:
    MUSTTAIL return op_print();
  case OP_DEFINE_GLOBAL:
    MUSTTAIL return op_define_global();
  case OP_GET_GLOBAL:
    MUSTTAIL return op_get_global();
  case OP_SET_GLOBAL:
    MUSTTAIL return op_set_global();
  case OP_GET_LOCAL:
    MUSTTAIL return op_get_local();
  case OP_SET_LOCAL:
    MUSTTAIL return op_set_local();
  case OP_GET_UPVALUE:
    MUSTTAIL return op_get_upvalue();
  case OP_SET_UPVALUE:
    MUSTTAIL return op_set_upvalue();
  case OP_POP:
    MUSTTAIL return op_pop();
  case OP_JUMP_IF_FALSE:
    MUSTTAIL return op_jump_if_false();
  case OP_JUMP:
    MUSTTAIL return op_jump();
  case OP_LOOP:
    MUSTTAIL return op_loop();
  case OP_CALL:
    MUSTTAIL return op_call();
  case OP_CLOSURE:
    MUSTTAIL return op_closure();
  }
  return INTERPRET_COMPILE_ERROR;
}

const InterpretResult VM::run() { MUSTTAIL return dispatch(); };

InterpretResult VM::op_return() {
  Value result = std::move(stack.back());
  stack.pop_back();
  frames.pop_back();
  if (frames.empty()) {
    stack.pop_back();
    return INTERPRET_OK;
  }
  // Erase all the called function's stack window.
  stack.erase(frame->slots, stack.end());
  stack.push_back(result);
  frame = &frames.back();
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_constant() {
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  const Value constant = frame->readConstant();
  stack.push_back(constant);
  // printValue(constant);
  // std::cout << "\n";
#ifdef BENCHMARK
  auto t2 = std::chrono::high_resolution_clock::now();
  /* Getting number of milliseconds as an integer. */
  auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
  times[OP_CONSTANT] += ms_int.count();
  calls[OP_CONSTANT]++;
#endif
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_call() {
  uint8_t argCount = frame->readByte();
  if (UNLIKELY(!callValue(stack[stack.size() - 1 - argCount], argCount))) {
    runtimeError("Call error");
    return INTERPRET_RUNTIME_ERROR;
  }
  frame = &frames.back();
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_less() {
  if (UNLIKELY(stack.size() < 2)) {
    runtimeError("Stack underflow.");
    return INTERPRET_RUNTIME_ERROR;
  }
  if (UNLIKELY(!(*(stack.end() - 1)).isNumber() &&
               !(*(stack.end() - 2)).isNumber())) {
    runtimeError("Operands must be numbers.");
    return INTERPRET_RUNTIME_ERROR;
  }
  double b = stack.back().asNumber();
  stack.pop_back();
  double a = stack.back().asNumber();
  stack.pop_back();
  stack.push_back(Value{a < b});
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_add() {
  if (UNLIKELY(stack.size() < 2)) {
    runtimeError("Stack underflow");
    return INTERPRET_RUNTIME_ERROR;
  }
  Value b = std::move(stack.back());
  stack.pop_back();
  Value a = std::move(stack.back());
  stack.pop_back();

  ValueType t_a = a.getType();
  ValueType t_b = b.getType();

  // TODO: Branch prediction
  if (t_a == ValueType::NUMBER && t_b == ValueType::NUMBER) {
    stack.push_back(Value{a.asNumber() + b.asNumber()});
  } else if (t_a == ValueType::STRING && t_b == ValueType::STRING) {
    const StringObj * const interned =
        stringIntern.intern(std::string{a.asObj()->as<StringObj>()->str} +
                            std::string{b.asObj()->as<StringObj>()->str});
    // TODO: Fix allocations
    stack.push_back(Value{interned});
  } else {
    runtimeError("Operands must be two numbers or two strings.");
    return INTERPRET_RUNTIME_ERROR;
  }

  MUSTTAIL return dispatch();
}

InterpretResult VM::op_subtract() {
  if (UNLIKELY(stack.size() < 2)) {
    runtimeError("Stack underflow");
    return INTERPRET_RUNTIME_ERROR;
  }
  if (UNLIKELY(!(stack.end() - 1)->isNumber() ||
               !(stack.end() - 2)->isNumber())) {
    runtimeError("Operands must be numbers");
    return INTERPRET_RUNTIME_ERROR;
  }

  // TODO: Calculate in place
  double b = stack.back().asNumber();
  stack.pop_back();
  double a = stack.back().asNumber();
  stack.pop_back();
  stack.emplace_back(a - b);
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_negate() {
  Value val = stack.back();
  if (UNLIKELY(!val.isNumber())) {
    runtimeError("Operand must be a number");
    return INTERPRET_RUNTIME_ERROR;
  }
  stack.pop_back();
  stack.emplace_back(-val.asNumber());
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_nil() {
  stack.push_back(Value{});
  MUSTTAIL return dispatch();
}
InterpretResult VM::op_true() {
  stack.push_back(Value{true});
  MUSTTAIL return dispatch();
}
InterpretResult VM::op_false() {
  stack.push_back(Value{false});
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_multiply() {
  auto res = binary_op([](double a, double b) constexpr { return a * b; });
  if (UNLIKELY(res == INTERPRET_RUNTIME_ERROR)) {
    return res;
  }

  MUSTTAIL return dispatch();
}

InterpretResult VM::op_pop() {
  ZoneScopedNC("POP", tracy::Color::CadetBlue);
  stack.pop_back();
  FrameMark;
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_get_global() {
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  const Value &value = frame->readConstantRef();
  if (value.getType() == ValueType::STRING) {
    const auto &variable =
        globals.find(value.asObj()->as<StringObj>()->str);
    if (variable != globals.end()) {
      stack.push_back(variable->second);
#ifdef BENCHMARK
      auto t2 = std::chrono::high_resolution_clock::now();
      /* Getting number of milliseconds as an integer. */
      auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
      times[OP_GET_GLOBAL] += ms_int.count();
      calls[OP_GET_GLOBAL]++;
#endif
      MUSTTAIL return dispatch();
    } else {
      runtimeError("Undefined variable '{}.'",
                   value.asObj()->as<StringObj>()->str);
      return INTERPRET_RUNTIME_ERROR;
    }
  }
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_set_global() {
  const Value &name = frame->readConstant();
  if (UNLIKELY(!name.isString())) {
    // TODO: Is this logic right?
    MUSTTAIL return dispatch();
  }
  const Value value = stack.back();
  const std::string_view nameStr = name.asObj()->as<StringObj>()->str;
  if (UNLIKELY(!globals.contains(nameStr))) {
    runtimeError("Undefined variable '{}'.", nameStr);
    return INTERPRET_RUNTIME_ERROR;
  }
  globals[nameStr] = value;
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_get_local() {
  ZoneScopedNC("GET LOCAL", tracy::Color::Pink);
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  uint8_t slot = frame->readByte();
  stack.push_back(frame->slots[slot]);
  FrameMark;
#ifdef BENCHMARK
  auto t2 = std::chrono::high_resolution_clock::now();
  /* Getting number of milliseconds as an integer. */
  auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
  times[OP_GET_LOCAL] += ms_int.count();
  calls[OP_GET_LOCAL]++;
#endif
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_set_local() {
  ZoneScopedNC("SET LOCAL", tracy::Color::Pink);
  uint8_t slot = frame->readByte();
  frame->slots[slot] = stack.back();
  FrameMark;
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_get_upvalue() {
  uint8_t slot = frame->readByte();
  stack.push_back(*(frame->closure->upvalues[slot].location));
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_set_upvalue() {
  uint8_t slot = frame->readByte();
  Value * val = frame->closure->upvalues[slot].location;
  val->set(stack.back());
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_define_global() {
  const Value &name = frame->readConstant();
  if (UNLIKELY(!name.isString())) {
    MUSTTAIL return dispatch();
  }
  const Value value = stack.back();
  const std::string_view nameStr = name.asObj()->as<StringObj>()->str;

  globals.insert_or_assign(nameStr, value);
  stack.pop_back();
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_equal() {
  Value a = std::move(stack.back());
  stack.pop_back();
  Value b = std::move(stack.back());
  stack.pop_back();
  stack.push_back(Value{valuesEqual(a, b)});
  MUSTTAIL return dispatch();
}
InterpretResult VM::op_greater() {
  auto res = binary_op([](double a, double b) constexpr { return a > b; });
  if (UNLIKELY(res == INTERPRET_RUNTIME_ERROR)) {
    return res;
  }
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_divide() {
  auto res = binary_op([](double a, double b) constexpr { return a / b; });
  if (UNLIKELY(res == INTERPRET_RUNTIME_ERROR)) {
    return res;
  }
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_not() {
  // TODO: In place
  Value &val = stack.back();
  stack.emplace_back(isFalsey(val));
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_print() {
  printValue(stack.back());
  std::cout << "\n";
  stack.pop_back();
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_jump() {
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  frame->ip() += frame->readShort();
#ifdef BENCHMARK
  auto t2 = std::chrono::high_resolution_clock::now();
  /* Getting number of milliseconds as an integer. */
  auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
  times[OP_JUMP] += ms_int.count();
  calls[OP_JUMP]++;
#endif
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_jump_if_false() {
  ZoneScopedNC("JUMP", tracy::Color::Black);
#ifdef BENCHMARK
  auto t1 = std::chrono::high_resolution_clock::now();
#endif
  uint16_t offset = frame->readShort();
  if (isFalsey(stack.back())) {
    frame->ip() += offset;
  }
#ifdef BENCHMARK
  auto t2 = std::chrono::high_resolution_clock::now();
  /* Getting number of milliseconds as an integer. */
  auto ms_int = duration_cast<std::chrono::nanoseconds>(t2 - t1);
  times[OP_JUMP_IF_FALSE] += ms_int.count();
  calls[OP_JUMP_IF_FALSE]++;
#endif
  FrameMark;
  MUSTTAIL return dispatch();
}
InterpretResult VM::op_loop() {
  uint16_t offset = frame->readShort();
  frame->ip() -= offset;
  MUSTTAIL return dispatch();
}

InterpretResult VM::op_closure() {
  // TODO: This ought to be refactored.
  // Need to be careful here, because we've mixed values, references,
  // pointers and smart pointers.
  {
    const Function *funPtr =
        frame->readConstantRef().asObj()->as<Function>();
    Closure closure{funPtr};
    for (int i = 0; i < closure.function->upvalueCount; i++) {
      uint8_t isLocal = frame->readByte();
      uint8_t index = frame->readByte();
      if (isLocal) {
        closure.upvalues.push_back(captureUpvalue(frame->slots + index));
      } else {
        closure.upvalues.push_back(frame->closure->upvalues[index]);
      }
    }
    closures.push_back(closure);
    stack.emplace_back(&closures.back());
  }
  MUSTTAIL return dispatch();
}
