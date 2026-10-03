#pragma once

#include <base/periodic_source.hpp>
#include <core/math/trig.hpp>

namespace push {

/**
 * PulseGen
 * @brief Generates a pulse signal with a configured duty cycle.
 * @image push.pulse-gen.svg
 */
template <typename I> class PulseGen : public PeriodicSource<I> {
public:
  using T = I::Value;

  /**
   * PulseGen
   * @param dutyCycle Duty cycle
   *   Fraction of the period during which the pulse signal is high.
   *   @icon duty_cycle.svg
   *   @control slider
   *   @min 0
   *   @max 1
   *   @step 0.01
   * @param amplitude Amplitude
   *   Pulse signal amplitude.
   *   @icon amplitude.svg
   *   @control number
   * @param frequency Frequency
   *   Frequency of the pulse signal in Hertz.
   *   @icon frequency.svg
   *   @control number
   *   @min 0
   *   @step 0.1
   * @param phase Phase
   *   Initial phase offset in radians.
   *   @icon phase.svg
   *   @control number
   */
  explicit PulseGen(const u32 blockId,
                    const T   dutyCycle = T{0.5},
                    const T   amplitude = 1,
                    const T   frequency = 1,
                    const T   phase = 0)
      : PeriodicSource<I>(blockId,
                          1),
        dutyCycle(dutyCycle),
        amplitude(amplitude),
        frequency(frequency),
        phase(phase) {}

  ~PulseGen() override = default;

  const T dutyCycle;
  const T amplitude;
  const T frequency;
  const T phase;

protected:
  void onStarted() override { t0 = get_time(); }

  T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<::f64>(get_time() - t0) * 0.001);
    const auto angle = math::wrapTwoPi(elapsedSec * frequency * math::kTwoPi<T> + phase);
    const auto progress = angle / math::kTwoPi<T>;
    return progress < dutyCycle ? amplitude : T{0};
  }

private:
  u64 t0{0};
};

} // namespace push
