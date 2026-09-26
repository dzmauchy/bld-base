#pragma once

#include <base/native_block.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * Scope
 * @brief Reports values received by independent scope channels.
 * @image scope.svg
 */
template <typename I, typename O>
class Scope : public NativeBlock<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = O::Value;

  explicit Scope(u32 blockId, u32 period = 60, u32 precision = 10) : NativeBlock<I, O>(blockId), period_(period), precision_(precision) {}

  [[nodiscard]] O apply(I input) override { return O{.channels = makeChannels(input.channelCount)}; }

  [[nodiscard]] auto period() const { return period_; }
  [[nodiscard]] auto precision() const { return precision_; }

 protected:
  using NativeBlock<I, O>::NativeBlock;

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
  Array<IndexedMemberConsumer<Scope<I, O>, T, &Scope<I, O>::handlePush>> channels_{};
};

}  // namespace push
