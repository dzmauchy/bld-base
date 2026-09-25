#pragma once

#include "blocks/periodic_source.hpp"
#include "math/trig.hpp"

namespace push {

template <typename T>
class PulseGen : public PeriodicSource<T> {
 public:
  ~PulseGen() override = default;

  [[nodiscard]] auto dutyCycle() const { return dutyCycle_; }
  [[nodiscard]] auto amplitude() const { return amplitude_; }
  [[nodiscard]] auto frequency() const { return frequency_; }
  [[nodiscard]] auto phase() const { return phase_; }

 protected:
  PulseGen(u32 blockId, T dutyCycle, T amplitude, T frequency, T phase)
      : PeriodicSource<T>(blockId, 1), dutyCycle_(dutyCycle), amplitude_(amplitude), frequency_(frequency), phase_(phase) {}

  void onStarted() override { t0_ = get_time(); }

  [[nodiscard]] T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<f64>(get_time() - t0_) * 0.001);
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
