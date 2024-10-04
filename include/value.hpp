#pragma once

#include <bit>
#include <bitset>
#include <cstdint>
#include <iostream>

// using nil = std::monostate;
// using Value =
//     std::variant<double, bool, nil, const std::string_view *, const Function
//     *,
//                  const NativeFunction *, const Closure *>;
//
// template <typename... Ts> struct overloaded : Ts... {
//   using Ts::operator()...;
// };
// template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
//
enum class ValueType {
  NUMBER,
  BOOL,
  NIL,
  FUNCTION,
  NATIVE_FUNCTION,
  CLOSURE,
  STRING,
  OBJ,
};

class Obj {
public:
protected:
  const ValueType type;

public:
  explicit Obj(ValueType type) : type(type) {}
  virtual ~Obj() = default;

  const ValueType getType() const { return type; }

  template <typename T> const T *const as() const {
    return static_cast<const T *>(this);
  }

  // Deleted copy operations to prevent slicing
  Obj(const Obj &) = delete;
  Obj &operator=(const Obj &) = delete;
};

class Value {
private:
  static constexpr uint64_t quiet_nan = 0x7FFC000000000000ULL;
  static constexpr uint64_t sign_bit = 0x8000000000000000ULL;

  static constexpr uint64_t tag_nil = 0x1;
  static constexpr uint64_t tag_false = 0x2;
  static constexpr uint64_t tag_true = 0x3;

  static constexpr uint64_t nil_val = quiet_nan | tag_nil;
  static constexpr uint64_t true_val = quiet_nan | tag_true;
  static constexpr uint64_t false_val = quiet_nan | tag_false;

public:
  // static constexpr value_t nil{};

  constexpr explicit Value() : _value{nil_val} {};
  constexpr explicit Value(double val) : _value{boxNumber(val)} {};
  constexpr explicit Value(Obj *val) : _value{boxObj(val)} {};
  constexpr explicit Value(const Obj *val) : _value{boxObj(val)} {};
  constexpr explicit Value(bool val) : _value{boxBool(val)} {};

  [[nodiscard]] const bool isNumber() const {
    return (_value & quiet_nan) != quiet_nan;
  }

  [[nodiscard]] const bool isBool() const { return (_value | 0x1) == true_val; }

  [[nodiscard]] const bool isNil() const { return _value == nil_val; }

  [[nodiscard]] const bool isObj() const {
    return (_value & (quiet_nan | sign_bit)) == (quiet_nan | sign_bit);
  }

  [[nodiscard]] const bool is(ValueType type) const {
    auto t = getType() ;
    if(t == type) {
      return true;
    }

    if(t == ValueType::OBJ) {
      if(t == this->asObj()->getType()) {
        return true;
      } else {
        return false;
      }
    }
    return false;
  }

  [[nodiscard]] const double asNumber() const {
    return std::bit_cast<double>(_value);
  };

  [[nodiscard]] const bool asBool() const { return _value == true_val; };

  [[nodiscard]] Obj *asObj() const {
    return reinterpret_cast<Obj *>(_value & ~(sign_bit | quiet_nan));
  };

  void setNumber(double val) { _value = boxNumber(val); };

  void setBool(bool val) { _value = boxBool(val); };

  void setObj(Obj *val) { _value = boxObj(val); };

  void setObj(const Obj *val) { _value = boxObj(val); };

  ValueType getType() const {
    if (isNumber()) {
      return ValueType::NUMBER;
    }
    if (isBool()) {
      return ValueType::BOOL;
    }
    if (isObj()) {
      return ValueType::OBJ;
    }
    return ValueType::NIL;
  }

  template <typename T, typename std::enable_if_t<std::is_same_v<T, double> ||
                                                      std::is_same_v<T, bool> ||
                                                      std::is_same_v<T, Obj *>,
                                                  int> = 0>
  T get() const {
    auto type = getType();
    if constexpr (std::is_same_v<T, double>) {
      if (type == ValueType::NUMBER) {
        return asNumber();
      }
      throw std::bad_cast();
    } else if constexpr (std::is_same_v<T, bool>) {
      if (type == ValueType::BOOL) {
        return asBool();
      }
      throw std::bad_cast();
    } else if constexpr (std::is_same_v<T, Obj *>) {
      if (type == ValueType::OBJ) {
        return asObj();
      }
    }
  }

  void print() { std::cout << std::bitset<64>(_value) << '\n'; }

private:
  uint64_t _value;

  static uint64_t boxNumber(double val) { return std::bit_cast<uint64_t>(val); }

  static uint64_t boxBool(bool val) { return val ? true_val : false_val; }

  static uint64_t boxObj(Obj *val) {
    return sign_bit | quiet_nan |
           static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val));
  }

  static uint64_t boxObj(const Obj *val) {
    return sign_bit | quiet_nan |
           static_cast<uint64_t>(reinterpret_cast<uintptr_t>(val));
  }

  // static uint64_t boxInt(int32_t val) {
  //   return static_cast<uint64_t>(val);
  // }
};

class StringObj : public Obj {

public:
  explicit StringObj(const std::string_view& str) : Obj{ValueType::STRING}, str{str} { };

  const std::string_view & str;

};

