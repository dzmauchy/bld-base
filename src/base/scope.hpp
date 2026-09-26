#pragma once

#include <base/native_block.hpp>

namespace push {

/**
 * Scope
 * @brief Reports values received by independent scope channels.
 * @image scope.svg
 */
template <typename I, typename O>
class Scope : public NativeBlock<I, O> {
 public:
  using T = O::Value;

  explicit Scope(const u32 blockId,
                 const u32 period = 60,
                 const u32 precision = 10)
      : NativeBlock<I,
                    O>(blockId),
        period_(period),
        precision_(precision) {}

  O apply(const I input) override { return O{.channels = makeChannels(input.channelCount)}; }

  const u32 period_;
  const u32 precision_;

 protected:
  using NativeBlock<I, O>::NativeBlock;

 private:
  void handlePush(const u8 channel,
                  const T  value) {
    this->sendValue(channel, value);
  }

  auto makeChannels(const u8 n) -> Vectorized<Consumer<T>> {
    channels_.clear();
    channels_.reserve(n);
    for (u8 i = 0; i < n; ++i) {
      channels_.emplace_back(this, i);
    }
    return this->template pointersOf<T>(channels_);
  }

  Array<IndexedMemberConsumer<Scope<I, O>, T, &Scope<I, O>::handlePush>> channels_{};
};

}  // namespace push
