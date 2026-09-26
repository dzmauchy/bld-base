#pragma once

#include <core/types.hpp>

/**
 * Function
 * @brief A callable that returns R from its argument values.
 * @image function.svg
 */
template <typename R, typename... Args>
class Function {
 public:
  virtual ~Function() = default;
  virtual R operator()(Args... args) = 0;
};

/**
 * Callback
 * @brief A callback with no arguments.
 * @image consumer.svg
 */
class Callback : public Consumer<> {
 public:
  ~Callback() override = default;
};

/**
 * MemberCallback
 * @brief Invokes a callback method on an existing object.
 * @image consumer.svg
 */
template <typename Target, void (Target::*Method)()>
class MemberCallback final : public Callback {
 public:
  constexpr explicit MemberCallback(Target* target) : target_(target) {}
  void operator()() override { (target_->*Method)(); }

 private:
  Target* target_;
};

/**
 * MemberConsumer
 * @brief Forwards one consumed value to an object method.
 * @image consumer.svg
 */
template <typename Target, typename T, void (Target::*Method)(T)>
class MemberConsumer final : public Consumer<T> {
 public:
  constexpr explicit MemberConsumer(Target* target) : target_(target) {}
  void operator()(T value) override { (target_->*Method)(value); }

 private:
  Target* target_;
};

/**
 * IndexedMemberConsumer
 * @brief Forwards a consumed value and channel index to an object method.
 * @image consumer.svg
 */
template <typename Target, typename T, void (Target::*Method)(u8, T)>
class IndexedMemberConsumer final : public Consumer<T> {
 public:
  constexpr IndexedMemberConsumer(Target* target, u8 index) : target_(target), index_(index) {}
  void operator()(T value) override { (target_->*Method)(index_, value); }

 private:
  Target* target_;
  u8 index_;
};
