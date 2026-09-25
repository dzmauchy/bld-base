#pragma once

#include <new>

#include <core/move.hpp>

template <typename T>
class Maybe {
 public:
  Maybe() = default;
  Maybe(T value) { emplace(move(value)); }
  Maybe(const Maybe&) = delete;
  Maybe& operator=(const Maybe&) = delete;
  Maybe(Maybe&& other) {
    if (other.has_) {
      emplace(move(*other.ptr()));
      other.reset();
    }
  }
  ~Maybe() { reset(); }

  template <typename... Args>
  T& emplace(Args&&... args) {
    reset();
    new (buf_) T(static_cast<Args&&>(args)...);
    has_ = true;
    return *ptr();
  }

  void reset() {
    if (has_) {
      ptr()->~T();
      has_ = false;
    }
  }

  explicit operator bool() const { return has_; }
  T& operator*() { return *ptr(); }
  const T& operator*() const { return *ptr(); }
  T* operator->() { return ptr(); }
  const T* operator->() const { return ptr(); }

 private:
  T* ptr() { return reinterpret_cast<T*>(buf_); }
  const T* ptr() const { return reinterpret_cast<const T*>(buf_); }

  alignas(T) unsigned char buf_[sizeof(T)]{};
  bool has_ = false;
};
