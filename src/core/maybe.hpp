#pragma once

#include <core/move.hpp>
#include <new>

/**
 * Maybe
 * @brief Optional owned storage for a value.
 * @image type.svg
 */
template <typename T> class Maybe {
public:
  Maybe() = default;
  Maybe(T value) { emplace(move(value)); }
  Maybe(const Maybe &) = delete;
  Maybe &operator=(const Maybe &) = delete;
  Maybe(Maybe &&other) {
    if (other.has) {
      emplace(move(*other.ptr()));
      other.reset();
    }
  }
  ~Maybe() { reset(); }

  template <typename... Args> T &emplace(Args &&...args) {
    reset();
    new (buf) T(static_cast<Args &&>(args)...);
    has = true;
    return *ptr();
  }

  void reset() {
    if (has) {
      ptr()->~T();
      has = false;
    }
  }

  explicit operator bool() const { return has; }
  T       &operator*() { return *ptr(); }
  const T &operator*() const { return *ptr(); }
  T       *operator->() { return ptr(); }
  const T *operator->() const { return ptr(); }

private:
  T *ptr() { return reinterpret_cast<T *>(buf); }
  [[nodiscard]]
  const T *ptr() const {
    return reinterpret_cast<const T *>(buf);
  }

  alignas(T) unsigned char buf[sizeof(T)]{};
  bool has = false;
};
