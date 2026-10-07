#pragma once

#include <base/periodic_source.hpp>
#include <functional>

namespace push::detail {

template <typename I>
std::function<void(I)> makeRandGen(const u32,
                                   const u32               precision,
                                   const typename I::Value amplitude) {
  using T = I::Value;
  return makePeriodicSource<I>(
      precision, [amplitude] { return random_of<T>() * amplitude; }, [] {});
}

} // namespace push::detail
