#pragma once

#include <core/hal.hpp>

namespace push::detail {

template <typename I>
core::function<void(I)> makeConstant(const u32,
                                     const typename I::Value value = 1) {
  using T = I::Value;
  auto downstream = core::make_shared<core::array<core::function<void(T)> *>>();
  auto start = core::make_shared<core::function<void()>>([downstream, value] {
    for (auto *sink : *downstream) {
      if (sink)
        (*sink)(value);
    }
  });
  return [downstream, start](I input) {
    *downstream =
        core::array<core::function<void(T)> *>(input.downstream.begin(), input.downstream.end());
    on_start(start.get());
  };
}

} // namespace push::detail
