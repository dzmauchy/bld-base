#pragma once

#include <core/types.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace push::detail {

template <typename I,
          typename O>
std::function<O(I)> makeUnaryTransformer(const u32,
                                         auto transform) {
  using T = I::Value;
  auto downstream = std::make_shared<std::vector<std::function<void(T)> *>>();
  auto consumer = std::make_shared<std::function<void(T)>>([downstream, transform](const T value) {
    const auto transformed = transform(value);
    for (auto *sink : *downstream) {
      if (sink)
        (*sink)(transformed);
    }
  });
  return [downstream, consumer](I input) {
    downstream->assign(input.downstream.begin(), input.downstream.end());
    return O{.consumer = consumer.get()};
  };
}

} // namespace push::detail
