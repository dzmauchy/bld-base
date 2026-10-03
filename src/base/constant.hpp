#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

/**
 * Constant
 * @brief Pushes a configured value when the runtime starts.
 * @image push.const.svg
 */
template <typename I> class Constant : public NativeBlock<I, void> {
public:
  using T = I::Value;

  /**
   * Constant
   * @param value Value
   *   Value emitted by the constant source.
   *   @icon push.const.svg
   *   @control number
   */
  explicit Constant(const u32 blockId,
                    const T   value = 1)
      : NativeBlock<I,
                    void>(blockId),
        value(value) {}

  void apply(I input) override {
    downstream = move(input.downstream);
    this->onStart(startCb);
  }

  const T value;

private:
  void handleStart() { this->pushTo(downstream, value); }

  VectorizedInput<Consumer<T>> downstream{};

  MemberConsumer<&Constant::handleStart> startCb{this};
};

} // namespace push
