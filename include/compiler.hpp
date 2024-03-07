#pragma once
#include "scanner.hpp"
#include <string_view>

class Compiler {
public:
  void compile(const std::string_view source) const noexcept;

private:
};