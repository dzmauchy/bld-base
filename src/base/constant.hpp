#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

/**
 * Constant
 * @brief Pushes a configured value when the runtime starts.
 * @image push.const.svg
 */
template <typename I>
class Constant : public NativeBlock<I, void> {
 public:
  using T = I::Value;

  explicit Constant(const u32 blockId,
                    const T   value = 1)
      : NativeBlock<I,
                    void>(blockId),
        value_(value) {}

  void apply(I input) override {
    downstream_ = move(input.downstream);
    this->onStart(startCb_);
  }

  const T value_;

 private:
  void handleStart() { this->pushTo(downstream_, value_); }

  Vectorized<Consumer<T>>                                downstream_{};
  MemberCallback<Constant<I>, &Constant<I>::handleStart> startCb_{this};
};

}  // namespace push
