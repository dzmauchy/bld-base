#pragma once

#include <base/periodic_source.hpp>
#include <core/math/trig.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * PulseGen
 * @brief Generates a pulse signal with a configured duty cycle.
 * @image push.pulse-gen.svg
 */
template <typename I, typename O>
class PulseGen : public PeriodicSource<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = I::Value;

  ~PulseGen() override = default;

  [[nodiscard]] auto dutyCycle() const { return dutyCycle_; }
  [[nodiscard]] auto amplitude() const { return amplitude_; }
  [[nodiscard]] auto frequency() const { return frequency_; }
  [[nodiscard]] auto phase() const { return phase_; }

 protected:
  PulseGen(u32 blockId, T dutyCycle, T amplitude, T frequency, T phase)
      : PeriodicSource<I, O>(blockId, 1), dutyCycle_(dutyCycle), amplitude_(amplitude), frequency_(frequency), phase_(phase) {}

  void onStarted() override { t0_ = get_time(); }

  [[nodiscard]] T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<::f64>(get_time() - t0_) * 0.001);
    const auto angle = math::wrapTwoPi(elapsedSec * frequency_ * math::kTwoPi<T> + phase_);
    const auto progress = angle / math::kTwoPi<T>;
    return progress < dutyCycle_ ? amplitude_ : T{0};
  }

 private:
  T dutyCycle_;
  T amplitude_;
  T frequency_;
  T phase_;
  u64 t0_{0};
};

}  // namespace push
