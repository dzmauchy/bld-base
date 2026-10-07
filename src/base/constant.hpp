#pragma once

#include <core/hal.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace push::detail {

template <typename I>
std::function<void(I)> makeConstant(const u32,
                                    const typename I::Value value = 1) {
  using T = I::Value;
  auto downstream = std::make_shared<std::vector<std::function<void(T)> *>>();
  auto start = std::make_shared<std::function<void()>>([downstream, value] {
    for (auto *sink : *downstream) {
      if (sink)
        (*sink)(value);
    }
  });
  return [downstream, start](I input) {
    downstream->assign(input.downstream.begin(), input.downstream.end());
    on_start(start.get());
  };
}

} // namespace push::detail
