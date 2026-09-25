#pragma once

#include <core/hal.hpp>
#include <core/types.hpp>

namespace math {

template <typename T>
inline constexpr T kTwoPi = T{0};

template <>
inline constexpr f32 kTwoPi<f32> = 2.f * 3.1415926f;

template <>
inline constexpr f64 kTwoPi<f64> = 2.0 * 3.14159265358979323846;

template <typename T>
[[nodiscard]] constexpr T wrapTwoPi(T angle) {
  const T full = kTwoPi<T>;
  while (angle >= full) {
    angle -= full;
  }
  while (angle < T{0}) {
    angle += full;
  }
  return angle;
}

[[nodiscard]] inline f32 sin(f32 value) { return ::sin_f32(value); }
[[nodiscard]] inline f64 sin(f64 value) { return ::sin_f64(value); }
[[nodiscard]] inline f32 cos(f32 value) { return ::cos_f32(value); }
[[nodiscard]] inline f64 cos(f64 value) { return ::cos_f64(value); }
[[nodiscard]] inline f32 tan(f32 value) { return ::tan_f32(value); }
[[nodiscard]] inline f64 tan(f64 value) { return ::tan_f64(value); }
[[nodiscard]] inline f32 asin(f32 value) { return ::asin_f32(value); }
[[nodiscard]] inline f64 asin(f64 value) { return ::asin_f64(value); }
[[nodiscard]] inline f32 acos(f32 value) { return ::acos_f32(value); }
[[nodiscard]] inline f64 acos(f64 value) { return ::acos_f64(value); }
[[nodiscard]] inline f32 atan(f32 value) { return ::atan_f32(value); }
[[nodiscard]] inline f64 atan(f64 value) { return ::atan_f64(value); }
[[nodiscard]] inline f32 exp(f32 value) { return ::exp_f32(value); }
[[nodiscard]] inline f64 exp(f64 value) { return ::exp_f64(value); }
[[nodiscard]] inline f32 log(f32 value) { return ::log_f32(value); }
[[nodiscard]] inline f64 log(f64 value) { return ::log_f64(value); }
[[nodiscard]] inline f32 log10(f32 value) { return ::log10_f32(value); }
[[nodiscard]] inline f64 log10(f64 value) { return ::log10_f64(value); }
[[nodiscard]] inline f32 pow(f32 base, f32 exponent) { return ::pow_f32(base, exponent); }
[[nodiscard]] inline f64 pow(f64 base, f64 exponent) { return ::pow_f64(base, exponent); }
[[nodiscard]] inline f32 sqrt(f32 value) { return ::sqrt_f32(value); }
[[nodiscard]] inline f64 sqrt(f64 value) { return ::sqrt_f64(value); }
[[nodiscard]] inline f32 ceil(f32 value) { return ::ceil_f32(value); }
[[nodiscard]] inline f64 ceil(f64 value) { return ::ceil_f64(value); }
[[nodiscard]] inline f32 floor(f32 value) { return ::floor_f32(value); }
[[nodiscard]] inline f64 floor(f64 value) { return ::floor_f64(value); }

}  // namespace math
