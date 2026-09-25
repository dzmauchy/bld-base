#pragma once

#include <core/types.hpp>

template <typename... Args>
class Consumer {
 public:
  virtual ~Consumer() = default;
  virtual void operator()(Args... args) = 0;
};

template <typename R, typename... Args>
class Function {
 public:
  virtual ~Function() = default;
  virtual R operator()(Args... args) = 0;
};

class Callback : public Consumer<> {
 public:
  ~Callback() override = default;
};

template <typename Target, void (Target::*Method)()>
class MemberCallback final : public Callback {
 public:
  constexpr explicit MemberCallback(Target* target) : target_(target) {}
  void operator()() override { (target_->*Method)(); }

 private:
  Target* target_;
};

template <typename Target, typename T, void (Target::*Method)(T)>
class MemberConsumer final : public Consumer<T> {
 public:
  constexpr explicit MemberConsumer(Target* target) : target_(target) {}
  void operator()(T value) override { (target_->*Method)(value); }

 private:
  Target* target_;
};

template <typename Target, typename T, void (Target::*Method)(u8, T)>
class IndexedMemberConsumer final : public Consumer<T> {
 public:
  constexpr IndexedMemberConsumer(Target* target, u8 index) : target_(target), index_(index) {}
  void operator()(T value) override { (target_->*Method)(index_, value); }

 private:
  Target* target_;
  u8 index_;
};
