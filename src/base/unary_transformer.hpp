#pragma once

#include <core/types.hpp>

namespace push::detail {

template <typename I,
          typename O>
core::function<O(I)> makeUnaryTransformer(const u32,
                                          auto transform) {
  using T = I::Value;
  auto downstream = core::make_shared<core::array<core::function<void(T)> *>>();
  auto consumer =
      core::make_shared<core::function<void(T)>>([downstream, transform](const T value) {
        const auto transformed = transform(value);
        for (auto *sink : *downstream) {
          if (sink)
            (*sink)(transformed);
        }
      });
  return [downstream, consumer](I input) {
    *downstream =
        core::array<core::function<void(T)> *>(input.downstream.begin(), input.downstream.end());
    return O{.consumer = consumer.get()};
  };
}

} // namespace push::detail
