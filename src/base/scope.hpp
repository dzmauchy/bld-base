#pragma once

#include <base/native_block.hpp>

namespace push {

/**
 * Scope
 * @brief Reports values received by independent scope channels.
 * @image scope.svg
 */
template <typename O> class Scope : public NativeBlock<void, O> {
public:
  using T = O::Value;

  /**
   * Scope
   * @param period Period
   *   Observation update period in seconds.
   *   @icon period.svg
   *   @control slider
   *   @min 1
   *   @max 3600
   *   @step 1
   * @param precision Precision
   *   Observation precision in milliseconds.
   *   @icon precision.svg
   *   @control number
   *   @min 1
   *   @max 1000
   *   @step 1
   */
  explicit Scope(const u32 blockId,
                 const u32 period = 60,
                 const u32 precision = 10)
      : NativeBlock<void,
                    O>(blockId),
        period(period),
        precision(precision) {}

  O apply() override { return O{.channels = VectorizedOutput<Consumer<T>>::template from<&Scope::makeChannels>(this)}; }

  const u32 period;
  const u32 precision;

protected:
  using NativeBlock<void, O>::NativeBlock;

private:
  void handlePush(const u8 channel,
                  const T  value) {
    this->sendValue(channel, value);
  }

  auto makeChannels(const u8 n) -> Array<Consumer<T> *> {
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
