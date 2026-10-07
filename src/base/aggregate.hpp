#pragma once

#include <cmath>
#include <core/hal.hpp>
#include <functional>
#include <limits>
#include <memory>
#include <vector>

namespace push::detail {

template <typename I,
          typename O>
std::function<O(I)> makeAggregate(const u32,
                                  const u32 precision,
                                  auto      combine) {
  using T = I::Value;
  auto downstream = std::make_shared<std::vector<std::function<void(T)> *>>();
  auto values = std::make_shared<std::vector<T>>();
  auto consumers = std::make_shared<std::vector<std::function<void(T)>>>();
  auto tick = std::make_shared<std::function<void()>>([downstream, values, combine] {
    if (values->empty())
      return;
    for (const auto value : *values) {
      if (!std::isfinite(value))
        return;
    }
    auto acc = (*values)[0];
    for (u32 i = 1; i < values->size(); ++i)
      acc = combine(acc, (*values)[i]);
    if (std::isfinite(acc)) {
      for (auto *sink : *downstream) {
        if (sink)
          (*sink)(acc);
      }
    }
  });
  auto close = std::make_shared<std::function<void()>>();
  auto start = std::make_shared<std::function<void()>>([precision, tick, close] {
    const auto timer = set_interval(precision, tick.get());
    *close = [timer] { clear_interval(timer); };
    on_close(close.get());
  });
  auto channelPointers = std::make_shared<std::vector<std::function<void(T)> *>>();
  VectorizedOutput<std::function<void(T)>> channels =
      [values, consumers, channelPointers,
       start](const u8 count) -> std::span<std::function<void(T)> *const> {
    values->assign(count, std::numeric_limits<T>::quiet_NaN());
    consumers->clear();
    consumers->reserve(count);
    channelPointers->clear();
    channelPointers->reserve(count);
    for (u8 i = 0; i < count; ++i) {
      consumers->emplace_back([values, i](const T value) { (*values)[i] = value; });
      channelPointers->push_back(&consumers->back());
    }
    on_start(start.get());
    return *channelPointers;
  };
  return [downstream, channels](I input) {
    downstream->assign(input.downstream.begin(), input.downstream.end());
    return O{.channels = channels};
  };
}

} // namespace push::detail
