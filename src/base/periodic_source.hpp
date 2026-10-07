#pragma once

#include <core/hal.hpp>

namespace push::detail {

template <typename I>
core::function<void(I)> makePeriodicSource(const u32 intervalMs,
                                           auto      sample,
                                           auto      onStarted) {
  using T = I::Value;
  auto downstream = core::make_shared<core::array<core::function<void(T)> *>>();
  auto tick = core::make_shared<core::function<void()>>([downstream, sample]() mutable {
    const auto value = sample();
    for (auto *sink : *downstream) {
      if (sink)
        (*sink)(value);
    }
  });
  auto close = core::make_shared<core::function<void()>>();
  auto start =
      core::make_shared<core::function<void()>>([intervalMs, tick, close, onStarted]() mutable {
        onStarted();
        const auto timer = set_interval(intervalMs, tick.get());
        *close = [timer] { clear_interval(timer); };
        on_close(close.get());
      });
  return [downstream, start](I input) {
    *downstream =
        core::array<core::function<void(T)> *>(input.downstream.begin(), input.downstream.end());
    on_start(start.get());
  };
}

} // namespace push::detail
