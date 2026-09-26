#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * UnaryTransformer
 * @brief Transforms each received value and pushes the result downstream.
 * @image transformer.svg
 */
template <typename I, typename O>
class UnaryTransformer : public NativeBlock<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = I::Value;

  ~UnaryTransformer() override = default;

  [[nodiscard]] O apply(I input) override {
    downstream_ = move(input.downstream);
    return O{.consumer = &pushConsumer_};
  }

 protected:
  using NativeBlock<I, O>::NativeBlock;
  [[nodiscard]] virtual T transform(T value) const = 0;

 private:
  void handlePush(T value) { this->pushTo(downstream_, transform(value)); }

  Vectorized<Consumer<T>> downstream_{};
  MemberConsumer<UnaryTransformer<I, O>, T, &UnaryTransformer<I, O>::handlePush> pushConsumer_{this};
};

}  // namespace push
