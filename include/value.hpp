#pragma once

#include "function.fwd.hpp"
#include <string_view>
#include <variant>

using nil = std::monostate;
using Value =
    std::variant<double, bool, nil, std::string_view, const Function *,
                 const NativeFunction *, const Closure *>;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

