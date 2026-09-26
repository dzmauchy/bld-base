#pragma once

#include <core/hal.hpp>
#include <core/types.hpp>

namespace math {

template <typename T> inline constexpr T kTwoPi = T{0};

template <> inline constexpr f32 kTwoPi<f32> = 2.f * 3.1415926f;

template <> inline constexpr f64 kTwoPi<f64> = 2.0 * 3.14159265358979323846;

template <typename T> constexpr T wrapTwoPi(T angle) {
  const T full = kTwoPi<T>;
  while (angle >= full) {
    angle -= full;
  }
  while (angle < T{0}) {
    angle += full;
  }
  return angle;
}

inline f32 sin(const f32 value) { return ::sin_f32(value); }
inline f64 sin(const f64 value) { return ::sin_f64(value); }
inline f32 cos(const f32 value) { return ::cos_f32(value); }
inline f64 cos(const f64 value) { return ::cos_f64(value); }
inline f32 tan(const f32 value) { return ::tan_f32(value); }
inline f64 tan(const f64 value) { return ::tan_f64(value); }
inline f32 asin(const f32 value) { return ::asin_f32(value); }
inline f64 asin(const f64 value) { return ::asin_f64(value); }
inline f32 acos(const f32 value) { return ::acos_f32(value); }
inline f64 acos(const f64 value) { return ::acos_f64(value); }
inline f32 atan(const f32 value) { return ::atan_f32(value); }
inline f64 atan(const f64 value) { return ::atan_f64(value); }
inline f32 exp(const f32 value) { return ::exp_f32(value); }
inline f64 exp(const f64 value) { return ::exp_f64(value); }
inline f32 log(const f32 value) { return ::log_f32(value); }
inline f64 log(const f64 value) { return ::log_f64(value); }
inline f32 log10(const f32 value) { return ::log10_f32(value); }
inline f64 log10(const f64 value) { return ::log10_f64(value); }
inline f32 pow(const f32 base,
               const f32 exponent) {
  return ::pow_f32(base, exponent);
}
inline f64 pow(const f64 base,
               const f64 exponent) {
  return ::pow_f64(base, exponent);
}
inline f32 sqrt(const f32 value) { return ::sqrt_f32(value); }
inline f64 sqrt(const f64 value) { return ::sqrt_f64(value); }
inline f32 ceil(const f32 value) { return ::ceil_f32(value); }
inline f64 ceil(const f64 value) { return ::ceil_f64(value); }
inline f32 floor(const f32 value) { return ::floor_f32(value); }
inline f64 floor(const f64 value) { return ::floor_f64(value); }

} // namespace math
