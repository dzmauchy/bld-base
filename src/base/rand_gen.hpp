#pragma once

#include <base/periodic_source.hpp>
#include <base/ports.hpp>
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
template <typename I, typename O>
class RandGen : public PeriodicSource<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = typename I::Value;

  ~RandGen() override = default;

  [[nodiscard]] auto precision() const { return this->intervalMs(); }
  [[nodiscard]] auto amplitude() const { return amplitude_; }

 protected:
  RandGen(u32 blockId, u32 precision, T amplitude) : PeriodicSource<I, O>(blockId, precision), amplitude_(amplitude) {}

  [[nodiscard]] T sample() override { return random_of<T>() * amplitude_; }

 private:
  T amplitude_;
};

}  // namespace push
