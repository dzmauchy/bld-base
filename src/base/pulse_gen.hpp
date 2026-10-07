#pragma once

#include <core/math.hpp>

#include <base/periodic_source.hpp>

namespace push::detail {

template <typename I>
core::function<void(I)> makePulseGen(const u32,
                                     const typename I::Value dutyCycle,
                                     const typename I::Value amplitude,
                                     const typename I::Value frequency,
                                     const typename I::Value phase) {
  using T = I::Value;
  auto startedAt = core::make_shared<u64>(0);
  return makePeriodicSource<I>(
      1,
      [startedAt, dutyCycle, amplitude, frequency, phase] {
        const auto elapsedSec = static_cast<T>(static_cast<f64>(get_time() - *startedAt) * 0.001);
        constexpr auto twoPi = T{2} * core::pi_v<T>;
        auto           angle = core::fmod(elapsedSec * frequency * twoPi + phase, twoPi);
        if (angle < T{0})
          angle += twoPi;
        return angle / twoPi < dutyCycle ? amplitude : T{0};
      },
      [startedAt] { *startedAt = get_time(); });
}

} // namespace push::detail
