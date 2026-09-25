#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

template <typename T>
class PeriodicSource : public NativeBlock {
 public:
  ~PeriodicSource() override = default;

  void apply(Vectorized<Pss<T>> downstream) {
    downstream_ = move(downstream);
    onStart(startCb_);
  }

 protected:
  PeriodicSource(u32 blockId, u32 intervalMs) : NativeBlock(blockId), intervalMs_(intervalMs) {}

  virtual void onStarted() {}
  [[nodiscard]] virtual T sample() = 0;

  [[nodiscard]] auto intervalMs() const { return intervalMs_; }

  Vectorized<Pss<T>> downstream_{};
  u32 intervalMs_;

 private:
  void handleTick() { pushTo(downstream_, sample()); }
  void handleStart() {
    onStarted();
    armInterval(intervalMs_, tickCb_, closeCb_);
  }

  MemberCallback<PeriodicSource<T>, &PeriodicSource<T>::handleTick> tickCb_{this};
  MemberCallback<PeriodicSource<T>, &PeriodicSource<T>::handleStart> startCb_{this};
  Maybe<ClearIntervalCallback> closeCb_{};
};

}  // namespace push
