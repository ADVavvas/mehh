#pragma once

#include "value.hpp"
#include <functional>
#include <string>
#include <unordered_set>

struct string_hash {
  using is_transparent = void;
  [[nodiscard]] size_t operator()(const char *txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(std::string_view txt) const {
    return std::hash<std::string_view>{}(txt);
  }
  [[nodiscard]] size_t operator()(const std::string &txt) const {
    return std::hash<std::string>{}(txt);
  }
};

class StringIntern {
private:
  // std::unordered_set<std::string, string_hash, std::equal_to<>> strings;
  std::unordered_set<StringObj, StringObj::hash> strings;

public:
  const StringObj* intern(const std::string &str) {
    auto it = strings.find(StringObj{str});
    if (it != strings.end()) {
      return &*it;
    }
    auto res = strings.emplace(str);
    return &*res.first;
  }
};
