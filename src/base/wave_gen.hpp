#pragma once

#include <base/periodic_source.hpp>
#include <core/math/trig.hpp>

namespace push {

/**
 * WaveGen
 * @brief Samples a periodic wave using runtime time.
 * @image wave.svg
 */
template <typename I>
class WaveGen : public PeriodicSource<I> {
 public:
  using T = I::Value;

  ~WaveGen() override = default;

  const T frequency_;
  const T amplitude_;
  const T phase_;

 protected:
  WaveGen(const u32 blockId,
          const u32 precision,
          const T   frequency,
          const T   amplitude,
          const T   phase)
      : PeriodicSource<I>(blockId,
                          precision),
        frequency_(frequency),
        amplitude_(amplitude),
        phase_(phase) {}

  void onStarted() override { t0_ = get_time(); }

  T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<::f64>(get_time() - t0_) * 0.001);
    const auto angle = math::wrapTwoPi(elapsedSec * frequency_ * math::kTwoPi<T> + phase_);
    return amplitude_ * wave(angle);
  }

  [[nodiscard]]
  virtual T wave(T angle) const = 0;

 private:
  u64 t0_{0};
};

}  // namespace push
