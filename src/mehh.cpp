#include "mehh.hpp"
#include "vm.hpp"
#include <fstream>
#include <iostream>

void Mehh::repl() noexcept {
  std::string line;

  std::cout << "\n> ";
  while (getline(std::cin, line)) {
    vm.interpret(line);
    std::cout << "\n> ";
  }
}

void Mehh::runFile(const std::string &path) noexcept {
  std::ifstream file{path};

  if (!file.is_open()) {
    std::cerr << "Could not open file: " << path << std::endl;
    return;
  }

  std::string source((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

  InterpretResult result = vm.interpret(source);

  file.close();
}