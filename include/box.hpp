#pragma once

#include "Tracy.hpp"
#include <memory>
#include <memory_resource>

#undef TRUE
#undef FALSE
// Wrapper for recursive std::variant
// (credit: https://www.foonathan.net/2022/05/recursive-variant-box/)
template <typename T> class box {

  static constexpr size_t BUFFER_SIZE = 1024 * 1024 * 8;
  inline static std::array<std::byte, BUFFER_SIZE> buffer;
  inline static std::pmr::monotonic_buffer_resource mbr{buffer.data(),
                                                        buffer.size()};
  inline static std::pmr::polymorphic_allocator<T> alloc{&mbr};

  class BoxDeleter {
  public:
    void operator()(T *ptr) {
      alloc.destroy(ptr);
      alloc.deallocate(ptr, 1);
    }
  };

  std::unique_ptr<T, BoxDeleter> _impl;

public:
  // box(T &&obj) : _impl(new T(std::move(obj))) {
  box(T &&obj) {
    ZoneScopedNC("box", tracy::Color::Violet);
    // _impl = std::make_unique<T>(std::move(obj));
    auto *raw = alloc.allocate(1);
    alloc.construct(raw, std::move(obj));
    _impl.reset(raw);
    FrameMark;
  }
  // box(const t &obj) : _impl(new t(obj)) {
  box(const T &obj) {
    ZoneScopedNC("box", tracy::Color::DarkViolet);
    // _impl = std::make_unique<T>(obj);
    auto *raw = alloc.allocate(1);
    alloc.construct(raw, obj);
    _impl.reset(raw);
    FrameMark;
  }

  box(const box &other) : box(*other._impl) {}
  box &operator=(const box &other) {
    *_impl = *other._impl;
    return *this;
  }

  ~box() = default;

  T *get() const { return _impl.get(); }

  T &operator*() { return *_impl; }
  const T &operator*() const { return *_impl; }

  T *operator->() { return _impl.get(); }
  const T *operator->() const { return _impl.get(); }

  bool operator==(const box &b) const {
    if (_impl == nullptr && b._impl == nullptr) {
      return true;
    }
    if (_impl == nullptr || b._impl == nullptr) {
      return false;
    }
    return *_impl == *b._impl;
  }

  bool operator!=(const box &b) const { return (*_impl != *b._impl); }
};
