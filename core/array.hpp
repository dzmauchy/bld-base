#pragma once

#include <initializer_list>
#include <new>

#include "core/move.hpp"
#include "core/types.hpp"

/**
 * <type id="array" name="Array">
 *   <description>An array of values</description>
 *   <arg name="T">
 *     <description>Array component type</description>
 *   </arg>
 * </type>
 */
template <typename T>
class Array {
 public:
  Array() = default;

  Array(std::initializer_list<T> values) {
    reserve(static_cast<u32>(values.size()));
    for (const auto& value : values) {
      push_back(value);
    }
  }

  Array(const Array& other) {
    reserve(other.size_);
    for (u32 i = 0; i < other.size_; ++i) {
      push_back(other.data_[i]);
    }
  }

  Array(Array&& other) noexcept : data_(other.data_), size_(other.size_), cap_(other.cap_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.cap_ = 0;
  }

  Array& operator=(const Array& other) {
    if (this == &other) {
      return *this;
    }
    reset();
    reserve(other.size_);
    for (u32 i = 0; i < other.size_; ++i) {
      push_back(other.data_[i]);
    }
    return *this;
  }

  Array& operator=(Array&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    reset();
    data_ = other.data_;
    size_ = other.size_;
    cap_ = other.cap_;
    other.data_ = nullptr;
    other.size_ = 0;
    other.cap_ = 0;
    return *this;
  }

  ~Array() { reset(); }

  void reserve(u32 n) {
    if (n <= cap_) {
      return;
    }
    auto* next = static_cast<T*>(::operator new(sizeof(T) * n));
    for (u32 i = 0; i < size_; ++i) {
      new (next + i) T(move(data_[i]));
      data_[i].~T();
    }
    if (data_) {
      ::operator delete(data_);
    }
    data_ = next;
    cap_ = n;
  }

  void push_back(const T& value) {
    grow();
    new (data_ + size_) T(value);
    ++size_;
  }

  void push_back(T&& value) {
    grow();
    new (data_ + size_) T(move(value));
    ++size_;
  }

  template <typename... Args>
  T& emplace_back(Args&&... args) {
    grow();
    new (data_ + size_) T(static_cast<Args&&>(args)...);
    ++size_;
    return data_[size_ - 1];
  }

  void assign(u32 n, const T& value) {
    reset();
    reserve(n);
    for (u32 i = 0; i < n; ++i) {
      push_back(value);
    }
  }

  void clear() {
    for (u32 i = 0; i < size_; ++i) {
      data_[i].~T();
    }
    size_ = 0;
  }

  T& operator[](u32 i) { return data_[i]; }
  const T& operator[](u32 i) const { return data_[i]; }
  [[nodiscard]] u32 size() const { return size_; }
  [[nodiscard]] bool empty() const { return size_ == 0; }
  T* begin() { return data_; }
  T* end() { return data_ + size_; }
  const T* begin() const { return data_; }
  const T* end() const { return data_ + size_; }

 private:
  void grow() {
    if (size_ == cap_) {
      reserve(cap_ == 0 ? 4u : cap_ * 2u);
    }
  }

  void reset() {
    clear();
    if (data_) {
      ::operator delete(data_);
      data_ = nullptr;
      cap_ = 0;
    }
  }

  T* data_ = nullptr;
  u32 size_ = 0;
  u32 cap_ = 0;
};

template <typename T>
[[nodiscard]] Array<T> arrayFrom(const T* items, u32 count) {
  auto result = Array<T>{};
  for (u32 i = 0; i < count; ++i) {
    result.push_back(items[i]);
  }
  return result;
}
