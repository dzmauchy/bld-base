#pragma once

#include <base/native_block.hpp>
#include <core/maybe.hpp>
#include <core/move.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * PeriodicSource
 * @brief Emits sampled values at a configured interval.
 * @image source.svg
 */
template <typename I, typename O>
class PeriodicSource : public NativeBlock<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = typename I::Value;

  static_assert(std::is_void_v<O>, "Push sources have no returned ports");

  ~PeriodicSource() override = default;

  O apply(I input) override {
    downstream_ = move(input.downstream);
    this->onStart(startCb_);
  }

 protected:
  PeriodicSource(u32 blockId, u32 intervalMs) : NativeBlock<I, O>(blockId), intervalMs_(intervalMs) {}

  virtual void onStarted() {}
  [[nodiscard]] virtual T sample() = 0;

  [[nodiscard]] auto intervalMs() const { return intervalMs_; }

  Vectorized<Consumer<T>> downstream_{};
  u32 intervalMs_;

 private:
  void handleTick() { this->pushTo(downstream_, sample()); }
  void handleStart() {
    onStarted();
    this->armInterval(intervalMs_, tickCb_, closeCb_);
  }

  MemberCallback<PeriodicSource<I, O>, &PeriodicSource<I, O>::handleTick> tickCb_{this};
  MemberCallback<PeriodicSource<I, O>, &PeriodicSource<I, O>::handleStart> startCb_{this};
  Maybe<typename NativeBlock<I, O>::ClearIntervalCallback> closeCb_{};
};

}  // namespace push
