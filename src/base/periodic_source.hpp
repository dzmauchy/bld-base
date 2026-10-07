#pragma once

#include <core/hal.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace push::detail {

template <typename I>
std::function<void(I)> makePeriodicSource(const u32 intervalMs,
                                          auto      sample,
                                          auto      onStarted) {
  using T = I::Value;
  auto downstream = std::make_shared<std::vector<std::function<void(T)> *>>();
  auto tick = std::make_shared<std::function<void()>>([downstream, sample]() mutable {
    const auto value = sample();
    for (auto *sink : *downstream) {
      if (sink)
        (*sink)(value);
    }
  });
  auto close = std::make_shared<std::function<void()>>();
  auto start =
      std::make_shared<std::function<void()>>([intervalMs, tick, close, onStarted]() mutable {
        onStarted();
        const auto timer = set_interval(intervalMs, tick.get());
        *close = [timer] { clear_interval(timer); };
        on_close(close.get());
      });
  return [downstream, start](I input) {
    downstream->assign(input.downstream.begin(), input.downstream.end());
    on_start(start.get());
  };
}

} // namespace push::detail
