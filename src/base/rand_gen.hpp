#pragma once

#include <base/periodic_source.hpp>
#include <core/hal.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
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

  ~RandGen() override = default;

  const T amplitude_;

 protected:
  RandGen(u32 blockId, u32 precision, T amplitude) : PeriodicSource<I>(blockId, precision), amplitude_(amplitude) {}

  [[nodiscard]] T sample() override { return random_of<T>() * amplitude_; }
};

}  // namespace push
