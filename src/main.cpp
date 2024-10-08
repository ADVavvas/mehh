#include "mehh.hpp"
#include <cstdlib>
#include <iostream>

#include "Tracy.hpp"

int main(int argc, char *argv[]) {
  ZoneScoped;
  Mehh mehh{};
  if (argc == 1) {
    mehh.repl();
  } else if (argc == 2) {
    mehh.runFile(argv[1]);
  } else {
    std::cerr << "Usage: mehh [path]\n";
    exit(64);
  }

  return 0;
}
