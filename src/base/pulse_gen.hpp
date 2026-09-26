#pragma once

#include <base/periodic_source.hpp>
#include <core/math/trig.hpp>

namespace push {

/**
 * PulseGen
 * @brief Generates a pulse signal with a configured duty cycle.
 * @image push.pulse-gen.svg
 */
template <typename I>
class PulseGen : public PeriodicSource<I> {
 public:
  using T = I::Value;

  explicit PulseGen(const u32 blockId,
                    const T   dutyCycle = T{0.5},
                    const T   amplitude = 1,
                    const T   frequency = 1,
                    const T   phase = 0)
      : PeriodicSource<I>(blockId,
                          1),
        dutyCycle_(dutyCycle),
        amplitude_(amplitude),
        frequency_(frequency),
        phase_(phase) {}

  ~PulseGen() override = default;

  const T dutyCycle_;
  const T amplitude_;
  const T frequency_;
  const T phase_;

 protected:
  void onStarted() override { t0_ = get_time(); }

  T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<::f64>(get_time() - t0_) * 0.001);
    const auto angle = math::wrapTwoPi(elapsedSec * frequency_ * math::kTwoPi<T> + phase_);
    const auto progress = angle / math::kTwoPi<T>;
    return progress < dutyCycle_ ? amplitude_ : T{0};
  }

 private:
  u64 t0_{0};
};

}  // namespace push
