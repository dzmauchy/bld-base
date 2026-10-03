#pragma once

#include <base/periodic_source.hpp>
#include <core/hal.hpp>

namespace push {

/**
 * RandGen
 * @brief Generates random values scaled by an amplitude.
 * @image push.rand-gen.svg
 */
template <typename I> class RandGen : public PeriodicSource<I> {
public:
  using T = I::Value;

  /**
   * RandGen
   * @param precision Precision
   *   Sampling interval in milliseconds.
   *   @icon precision.svg
   *   @control number
   *   @min 1
   *   @max 1000
   *   @step 1
   * @param amplitude Amplitude
   *   Scale factor applied to generated random values.
   *   @icon amplitude.svg
   *   @control number
   */
  explicit RandGen(const u32 blockId,
                   const u32 precision = 10,
                   const T   amplitude = 1)
      : PeriodicSource<I>(blockId,
                          precision),
        amplitude(amplitude) {}

  ~RandGen() override = default;

  const T amplitude;

protected:
  T sample() override { return random_of<T>() * amplitude; }
};

} // namespace push
