#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

template <typename T>
class Constant : public NativeBlock {
 public:
  explicit Constant(u32 blockId, T value) : NativeBlock(blockId), value_(value) {}

  void apply(VectorizedInput<Pss<T>> downstream) {
    downstream_ = move(downstream);
    onStart(startCb_);
  }

  [[nodiscard]] auto value() const { return value_; }

 private:
  void handleStart() { pushTo(downstream_, value_); }

  T value_;
  VectorizedInput<Pss<T>> downstream_{};
  MemberCallback<Constant<T>, &Constant<T>::handleStart> startCb_{this};
};

}  // namespace push
