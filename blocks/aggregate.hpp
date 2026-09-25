#pragma once

#include "blocks/native_block.hpp"
#include "core/hal.hpp"
#include "core/move.hpp"

namespace push {

template <typename T>
class Aggregate : public NativeBlock {
 public:
  ~Aggregate() override = default;

  [[nodiscard]] auto apply(VectorizedInput<Pss<T>> downstream, u8 n) {
    downstream_ = move(downstream);
    return bindInputs(n);
  }

  [[nodiscard]] auto precision() const { return precision_; }

 protected:
  explicit Aggregate(u32 blockId, u32 precision = 10) : NativeBlock(blockId), precision_(precision) {}
  [[nodiscard]] virtual T combine(T acc, T value) const = 0;

 private:
  void handleChannel(u8 index, T value) { values_[index] = value; }

  void handleTick() { emitIfFinite(); }

  void handleStart() { armInterval(precision_, tickCb_, closeCb_); }

  [[nodiscard]] auto bindInputs(u8 n) -> VectorizedInput<Pss<T>> {
    values_.assign(n, nan_of<T>());
    inputs_.clear();
    inputs_.reserve(n);
    for (u8 i = 0; i < n; ++i) {
      inputs_.emplace_back(this, i);
    }
    onStart(startCb_);
    return pointersOf<T>(inputs_);
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
      pushTo(downstream_, acc);
    }
  }

  u32 precision_;
  VectorizedInput<Pss<T>> downstream_{};
  Array<T> values_{};
  Array<IndexedMemberConsumer<Aggregate<T>, T, &Aggregate<T>::handleChannel>> inputs_{};
  MemberCallback<Aggregate<T>, &Aggregate<T>::handleTick> tickCb_{this};
  MemberCallback<Aggregate<T>, &Aggregate<T>::handleStart> startCb_{this};
  Maybe<ClearIntervalCallback> closeCb_{};
};

}  // namespace push
