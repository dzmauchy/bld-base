#pragma once

#include <base/native_block.hpp>

namespace push {

template <typename T>
class Scope : public NativeBlock {
 public:
  explicit Scope(u32 blockId, u32 period = 60, u32 precision = 10) : NativeBlock(blockId), period_(period), precision_(precision) {}

  [[nodiscard]] auto apply(u8 n) { return makeChannels(n); }

  [[nodiscard]] auto period() const { return period_; }
  [[nodiscard]] auto precision() const { return precision_; }

 protected:
  using NativeBlock::NativeBlock;

 private:
  void handlePush(u8 channel, T value) { this->sendValue(channel, value); }

  [[nodiscard]] auto makeChannels(u8 n) -> Vectorized<Consumer<T>> {
    channels_.clear();
    channels_.reserve(n);
    for (u8 i = 0; i < n; ++i) {
      channels_.emplace_back(this, i);
    }
    return this->template pointersOf<T>(channels_);
  }

  u32 period_;
  u32 precision_;
  Array<IndexedMemberConsumer<Scope<T>, T, &Scope<T>::handlePush>> channels_{};
};

}  // namespace push
