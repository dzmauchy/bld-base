#pragma once

#include <base/native_block.hpp>
#include <core/hal.hpp>
#include <core/maybe.hpp>
#include <core/move.hpp>

namespace push {

template <typename I, typename O> class Aggregate : public NativeBlock<I, O> {
public:
  using T = I::Value;

  explicit Aggregate(const u32 blockId,
                     const u32 precision = 10)
      : NativeBlock<I,
                    O>(blockId),
        precision(precision) {}

  ~Aggregate() override = default;

  O apply(I input) override {
    downstream = move(input.downstream);
    return O{.channels = bindInputs(input.channelCount)};
  }

  const u32 precision;

protected:
  [[nodiscard]]
  virtual T combine(T acc,
                    T value) const = 0;

private:
  void handleChannel(const u8 index,
                     const T  value) {
    values[index] = value;
  }

  void handleTick() { emitIfFinite(); }

  void handleStart() { this->armInterval(precision, tickCb, closeCb); }

  auto bindInputs(const u8 n) -> Vectorized<Consumer<T>> {
    values.assign(n, nan_of<T>());
    inputs.clear();
    inputs.reserve(n);
    for (u8 i = 0; i < n; ++i) {
      inputs.emplace_back(this, i);
    }
    this->onStart(startCb);
    return this->template pointersOf<T>(inputs);
  }

  void emitIfFinite() const {
    if (values.empty()) {
      return;
    }
    for (auto value : values) {
      if (!is_finite(value)) {
        return;
      }
    }
    auto acc = values[0];
    for (u32 i = 1; i < values.size(); ++i) {
      acc = combine(acc, values[i]);
    }
    if (is_finite(acc)) {
      this->pushTo(downstream, acc);
    }
  }

  Vectorized<Consumer<T>>                                  downstream{};
  Array<T>                                                 values{};
  Array<IndexedMemberConsumer<&Aggregate::handleChannel>>  inputs{};
  MemberConsumer<&Aggregate::handleTick>                   tickCb{this};
  MemberConsumer<&Aggregate::handleStart>                  startCb{this};
  Maybe<typename NativeBlock<I, O>::ClearIntervalCallback> closeCb{};
};

} // namespace push
