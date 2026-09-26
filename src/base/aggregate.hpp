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
        precision_(precision) {}

  ~Aggregate() override = default;

  O apply(I input) override {
    downstream_ = move(input.downstream);
    return O{.channels = bindInputs(input.channelCount)};
  }

  const u32 precision_;

protected:
  [[nodiscard]]
  virtual T combine(T acc,
                    T value) const = 0;

private:
  void handleChannel(const u8 index,
                     const T  value) {
    values_[index] = value;
  }

  void handleTick() { emitIfFinite(); }

  void handleStart() { this->armInterval(precision_, tickCb_, closeCb_); }

  auto bindInputs(const u8 n) -> Vectorized<Consumer<T>> {
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

  Vectorized<Consumer<T>>                                               downstream_{};
  Array<T>                                                              values_{};
  Array<IndexedMemberConsumer<Aggregate, T, &Aggregate::handleChannel>> inputs_{};
  MemberCallback<Aggregate, &Aggregate::handleTick>                     tickCb_{this};
  MemberCallback<Aggregate, &Aggregate::handleStart>                    startCb_{this};
  Maybe<typename NativeBlock<I, O>::ClearIntervalCallback>              closeCb_{};
};

} // namespace push
