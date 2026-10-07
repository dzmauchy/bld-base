#pragma once

#include <base/periodic_source.hpp>

namespace push::detail {

template <typename I>
core::function<void(I)> makeRandGen(const u32,
                                    const u32               precision,
                                    const typename I::Value amplitude) {
  using T = I::Value;
  return makePeriodicSource<I>(
      precision, [amplitude] { return random_of<T>() * amplitude; }, [] {});
}

} // namespace push::detail
