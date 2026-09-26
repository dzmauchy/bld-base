#pragma once

#include <base/native_block.hpp>

namespace push {

/**
 * Scope
 * @brief Reports values received by independent scope channels.
 * @image scope.svg
 */
template <typename I, typename O> class Scope : public NativeBlock<I, O> {
public:
  using T = O::Value;

  explicit Scope(const u32 blockId,
                 const u32 period = 60,
                 const u32 precision = 10)
      : NativeBlock<I,
                    O>(blockId),
        period(period),
        precision(precision) {}

  O apply(const I input) override { return O{.channels = makeChannels(input.channelCount)}; }

  const u32 period;
  const u32 precision;

protected:
  using NativeBlock<I, O>::NativeBlock;

private:
  void handlePush(const u8 channel,
                  const T  value) {
    this->sendValue(channel, value);
  }

  auto makeChannels(const u8 n) -> Vectorized<Consumer<T>> {
    channels.clear();
    channels.reserve(n);
    for (u8 i = 0; i < n; ++i) {
      channels.emplace_back(this, i);
    }
    return this->template pointersOf<T>(channels);
  }

  Array<IndexedMemberConsumer<&Scope::handlePush>> channels{};
};

} // namespace push
