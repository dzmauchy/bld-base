#pragma once

#include <base/native_block.hpp>
#include <core/maybe.hpp>
#include <core/move.hpp>

namespace push {

/**
 * PeriodicSource
 * @brief Emits sampled values at a configured interval.
 * @image source.svg
 */
template <typename I> class PeriodicSource : public NativeBlock<I, void> {
public:
  using T = I::Value;

  ~PeriodicSource() override = default;

  void apply(I input) override {
    downstream = move(input.downstream);
    this->onStart(startCb);
  }

  const u32 intervalMs;

protected:
  PeriodicSource(const u32 blockId,
                 const u32 intervalMs)
      : NativeBlock<I,
                    void>(blockId),
        intervalMs(intervalMs) {}

  virtual void onStarted() {}
  virtual T    sample() = 0;

  Vectorized<Consumer<T>> downstream{};

private:
  void handleTick() { this->pushTo(downstream, sample()); }
  void handleStart() {
    onStarted();
    this->armInterval(intervalMs, tickCb, closeCb);
  }

  MemberConsumer<&PeriodicSource::handleTick>                 tickCb{this};
  MemberConsumer<&PeriodicSource::handleStart>                startCb{this};
  Maybe<typename NativeBlock<I, void>::ClearIntervalCallback> closeCb{};
};

} // namespace push
