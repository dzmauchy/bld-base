#pragma once

#include <core/math.hpp>

#include <core/hal.hpp>

namespace push::detail {

template <typename I,
          typename O>
core::function<O(I)> makeAggregate(const u32,
                                   const u32 precision,
                                   auto      combine) {
  using T = I::Value;
  auto downstream = core::make_shared<core::array<core::function<void(T)> *>>();
  auto values = core::make_shared<core::array<T>>();
  auto consumers = core::make_shared<core::array<core::function<void(T)>>>();
  auto tick = core::make_shared<core::function<void()>>([downstream, values, combine] {
    if (values->empty())
      return;
    for (const auto value : *values) {
      if (!core::isfinite(value))
        return;
    }
    auto acc = (*values)[0];
    for (u32 i = 1; i < values->size(); ++i)
      acc = combine(acc, (*values)[i]);
    if (core::isfinite(acc)) {
      for (auto *sink : *downstream) {
        if (sink)
          (*sink)(acc);
      }
    }
  });
  auto close = core::make_shared<core::function<void()>>();
  auto start = core::make_shared<core::function<void()>>([precision, tick, close] {
    const auto timer = set_interval(precision, tick.get());
    *close = [timer] { clear_interval(timer); };
    on_close(close.get());
  });
  auto channelPointers = core::make_shared<core::array<core::function<void(T)> *>>();
  VectorizedOutput<core::function<void(T)>> channels =
      [values, consumers, channelPointers,
       start](const u8 count) -> core::span<core::function<void(T)> *const> {
    *values = core::array<T>(count, core::quiet_nan<T>());
    *consumers = core::array<core::function<void(T)>>(count);
    *channelPointers = core::array<core::function<void(T)> *>(count);
    for (u8 i = 0; i < count; ++i) {
      (*consumers)[i] = [values, i](const T value) { (*values)[i] = value; };
      (*channelPointers)[i] = &(*consumers)[i];
    }
    on_start(start.get());
    return *channelPointers;
  };
  return [downstream, channels](I input) {
    *downstream =
        core::array<core::function<void(T)> *>(input.downstream.begin(), input.downstream.end());
    return O{.channels = channels};
  };
}

} // namespace push::detail
