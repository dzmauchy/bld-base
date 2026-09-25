#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

template <typename T>
class UnaryTransformer : public NativeBlock {
 public:
  ~UnaryTransformer() override = default;

  [[nodiscard]] auto apply(Vectorized<Consumer<T>> downstream) {
    downstream_ = move(downstream);
    return &pushConsumer_;
  }

 protected:
  using NativeBlock::NativeBlock;
  [[nodiscard]] virtual T transform(T value) const = 0;

 private:
  void handlePush(T value) { pushTo(downstream_, transform(value)); }

  Vectorized<Consumer<T>> downstream_{};
  MemberConsumer<UnaryTransformer<T>, T, &UnaryTransformer<T>::handlePush> pushConsumer_{this};
};

}  // namespace push
