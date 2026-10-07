#pragma once

#include <base/periodic_source.hpp>
#include <cmath>
#include <functional>
#include <numbers>

namespace push::detail {

template <typename I>
std::function<void(I)> makeWaveGen(const u32,
                                   const u32               precision,
                                   const typename I::Value frequency,
                                   const typename I::Value amplitude,
                                   const typename I::Value phase,
                                   auto                    wave) {
  using T = I::Value;
  auto startedAt = std::make_shared<u64>(0);
  return makePeriodicSource<I>(
      precision,
      [startedAt, frequency, amplitude, phase, wave] {
        const auto elapsedSec = static_cast<T>(static_cast<f64>(get_time() - *startedAt) * 0.001);
        constexpr auto twoPi = T{2} * std::numbers::pi_v<T>;
        auto           angle = std::fmod(elapsedSec * frequency * twoPi + phase, twoPi);
        if (angle < T{0})
          angle += twoPi;
        return amplitude * wave(angle);
      },
      [startedAt] { *startedAt = get_time(); });
}

} // namespace push::detail
