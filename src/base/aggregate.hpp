#pragma once

#include <base/native_block.hpp>
#include <base/ports.hpp>
#include <core/hal.hpp>
#include <core/maybe.hpp>
#include <core/move.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * Aggregate
 * @brief Combines the latest finite values on a timer.
 * @image aggregate.svg
 */
template <typename I, typename O>
class Aggregate : public NativeBlock<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = typename I::Value;

  ~Aggregate() override = default;

  [[nodiscard]] O apply(I input) override {
    downstream_ = move(input.downstream);
    return O{.channels = bindInputs(input.channelCount)};
  }

  [[nodiscard]] auto precision() const { return precision_; }

 protected:
  explicit Aggregate(u32 blockId, u32 precision = 10) : NativeBlock<I, O>(blockId), precision_(precision) {}
  [[nodiscard]] virtual T combine(T acc, T value) const = 0;

 private:
  void handleChannel(u8 index, T value) { values_[index] = value; }

  void handleTick() { emitIfFinite(); }

  void handleStart() { this->armInterval(precision_, tickCb_, closeCb_); }

  [[nodiscard]] auto bindInputs(u8 n) -> Vectorized<Consumer<T>> {
    values_.assign(n, nan_of<T>());
    inputs_.clear();
    inputs_.reserve(n);
    for (u8 i = 0; i < n; ++i) {
      inputs_.emplace_back(this, i);
    }
    this->onStart(startCb_);
    return this->template pointersOf<T>(inputs_);
  }

  void emitIfFinite() const {
    if (values_.empty()) {
      return;
    }
    for (auto value : values_) {
      if (!is_finite(value)) {
        return;
      }
    }
    auto acc = values_[0];
    for (u32 i = 1; i < values_.size(); ++i) {
      acc = combine(acc, values_[i]);
    }
    if (is_finite(acc)) {
      this->pushTo(downstream_, acc);
    }
  }

  u32 precision_;
  Vectorized<Consumer<T>> downstream_{};
  Array<T> values_{};
  Array<IndexedMemberConsumer<Aggregate<I, O>, T, &Aggregate<I, O>::handleChannel>> inputs_{};
  MemberCallback<Aggregate<I, O>, &Aggregate<I, O>::handleTick> tickCb_{this};
  MemberCallback<Aggregate<I, O>, &Aggregate<I, O>::handleStart> startCb_{this};
  Maybe<typename NativeBlock<I, O>::ClearIntervalCallback> closeCb_{};
};

}  // namespace push
