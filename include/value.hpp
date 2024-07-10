#pragma once

#include "box.hpp"
#include "function.fwd.hpp"
#include <string_view>
#include <variant>

using nil = std::monostate;
using Value = std::variant<double, bool, nil, std::string_view, box<Function>, box<NativeFunction>, box<Closure>, UpvalueObj>;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
