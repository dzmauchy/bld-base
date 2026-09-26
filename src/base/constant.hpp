#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * Constant
 * @brief Pushes a configured value when the runtime starts.
 * @image push.const.svg
 */
template <typename I, typename O>
class Constant : public NativeBlock<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = typename I::Value;

  static_assert(std::is_void_v<O>, "Push sources have no returned ports");

  explicit Constant(u32 blockId, T value) : NativeBlock<I, O>(blockId), value_(value) {}

  O apply(I input) override {
    downstream_ = move(input.downstream);
    this->onStart(startCb_);
  }

  [[nodiscard]] auto value() const { return value_; }

 private:
  void handleStart() { this->pushTo(downstream_, value_); }

  T value_;
  Vectorized<Consumer<T>> downstream_{};
  MemberCallback<Constant<I, O>, &Constant<I, O>::handleStart> startCb_{this};
};

}  // namespace push
