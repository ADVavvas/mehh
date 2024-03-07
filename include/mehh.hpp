#pragma once

#include "vm.hpp"
#include <string>

class Mehh {
public:
  void repl() noexcept;
  void runFile(const std::string &path) noexcept;

private:
  VM vm{};
};