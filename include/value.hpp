#pragma once

#include "box.hpp"
#include "function.fwd.hpp"
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

using nil = std::monostate;
using Value = std::variant<double, bool, nil, std::string_view, box<Function>>;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
