#pragma once

#include <base/periodic_source.hpp>
#include <core/math/trig.hpp>

namespace push {

template <typename T>
class WaveGen : public PeriodicSource<T> {
 public:
  ~WaveGen() override = default;

  [[nodiscard]] auto precision() const { return this->intervalMs(); }
  [[nodiscard]] auto frequency() const { return frequency_; }
  [[nodiscard]] auto amplitude() const { return amplitude_; }
  [[nodiscard]] auto phase() const { return phase_; }

 protected:
  WaveGen(u32 blockId, u32 precision, T frequency, T amplitude, T phase)
      : PeriodicSource<T>(blockId, precision), frequency_(frequency), amplitude_(amplitude), phase_(phase) {}

  void onStarted() override { t0_ = get_time(); }

  [[nodiscard]] T sample() override {
    const auto elapsedSec = static_cast<T>(static_cast<f64>(get_time() - t0_) * 0.001);
    const auto angle = math::wrapTwoPi(elapsedSec * frequency_ * math::kTwoPi<T> + phase_);
    return amplitude_ * wave(angle);
  }

  [[nodiscard]] virtual T wave(T angle) const = 0;

 private:
  T frequency_;
  T amplitude_;
  T phase_;
  u64 t0_{0};
};

}  // namespace push
