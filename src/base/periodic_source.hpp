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
template <typename I>
class PeriodicSource : public NativeBlock<I, void> {
 public:
  using T = I::Value;

  ~PeriodicSource() override = default;

  void apply(I input) override {
    downstream_ = move(input.downstream);
    this->onStart(startCb_);
  }

  const u32 intervalMs_;

 protected:
  PeriodicSource(const u32 blockId,
                 const u32 intervalMs)
      : NativeBlock<I,
                    void>(blockId),
        intervalMs_(intervalMs) {}

  virtual void onStarted() {}
  virtual T    sample() = 0;

  Vectorized<Consumer<T>> downstream_{};

 private:
  void handleTick() { this->pushTo(downstream_, sample()); }
  void handleStart() {
    onStarted();
    this->armInterval(intervalMs_, tickCb_, closeCb_);
  }

  MemberCallback<PeriodicSource, &PeriodicSource::handleTick>  tickCb_{this};
  MemberCallback<PeriodicSource, &PeriodicSource::handleStart> startCb_{this};
  Maybe<typename NativeBlock<I, void>::ClearIntervalCallback>  closeCb_{};
};

}  // namespace push
