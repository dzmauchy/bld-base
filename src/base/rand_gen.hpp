#pragma once

#include <base/periodic_source.hpp>
#include <core/hal.hpp>

namespace push {

/**
 * RandGen
 * @brief Generates random values scaled by an amplitude.
 * @image push.rand-gen.svg
 */
template <typename I>
class RandGen : public PeriodicSource<I> {
 public:
  using T = I::Value;

  explicit RandGen(const u32 blockId,
                   const u32 precision = 10,
                   const T   amplitude = 1)
      : PeriodicSource<I>(blockId,
                          precision),
        amplitude_(amplitude) {}

  ~RandGen() override = default;

  const T amplitude_;

 protected:
  T sample() override { return random_of<T>() * amplitude_; }
};

}  // namespace push
