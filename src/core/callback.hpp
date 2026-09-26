#pragma once

#include <core/types.hpp>

using Callback = Consumer<>;

template <auto Method> class MemberConsumer;

template <typename Target, typename... Args, void (Target::*Method)(Args...)>
class MemberConsumer<Method> final : public Consumer<Args...> {
public:
  constexpr explicit MemberConsumer(Target *const target) : target(target) {}
  void operator()(Args... args) override { (target->*Method)(args...); }

private:
  Target *target;
};

template <auto Method> class IndexedMemberConsumer;

template <typename Target, typename... Args, void (Target::*Method)(u8, Args...)>
class IndexedMemberConsumer<Method> final : public Consumer<Args...> {
public:
  constexpr IndexedMemberConsumer(Target *const target,
                                  const u8      index)
      : target(target),
        index(index) {}
  void operator()(Args... args) override { (target->*Method)(index, args...); }

private:
  Target *target;
  u8      index;
};
