#pragma once

#include <base/periodic_source.hpp>
#include <core/math/trig.hpp>

namespace push {

/**
 * WaveGen
 * @brief Samples a periodic wave using runtime time.
 * @image wave.svg
 */
template <typename I> class WaveGen : public PeriodicSource<I> {
public:
  using T = I::Value;

  explicit WaveGen(const u32 blockId,
                   const u32 precision = 10,
                   const T   frequency = 1,
                   const T   amplitude = 1,
                   const T   phase = 0)
      : PeriodicSource<I>(blockId,
                          precision),
        frequency(frequency),
        amplitude(amplitude),
        phase(phase) {}

  ~WaveGen() override = default;

  const T frequency;
  const T amplitude;
  const T phase;

protected:
  void onStarted() override { t0 = get_time(); }

  T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<::f64>(get_time() - t0) * 0.001);
    const auto angle = math::wrapTwoPi(elapsedSec * frequency * math::kTwoPi<T> + phase);
    return amplitude * wave(angle);
  }

  [[nodiscard]]
  virtual T wave(T angle) const = 0;

private:
  u64 t0{0};
};

} // namespace push
