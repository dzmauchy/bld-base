#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

/**
 * UnaryTransformer
 * @brief Transforms each received value and pushes the result downstream.
 * @image transformer.svg
 */
template <typename I, typename O> class UnaryTransformer : public NativeBlock<I, O> {
public:
  using T = I::Value;
  using NativeBlock<I, O>::NativeBlock;

  ~UnaryTransformer() override = default;

  O apply(I input) override {
    downstream = move(input.downstream);
    return O{.consumer = &pushConsumer};
  }

protected:
  [[nodiscard]]
  virtual T transform(T value) const = 0;

private:
  void handlePush(const T value) { this->pushTo(downstream, transform(value)); }

  VectorizedInput<Consumer<T>>                  downstream{};
  MemberConsumer<&UnaryTransformer::handlePush> pushConsumer{this};
};

} // namespace push
