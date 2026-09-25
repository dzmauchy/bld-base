#pragma once

#include <core/callback.hpp>
#include <core/types.hpp>

extern "C" {
/* life-cycle callbacks */
void on_close(Callback* cbk);
void on_start(Callback* cbk);
void on_stop(Callback* cbk);

/* interval management */
[[nodiscard]] u32 set_interval(u32 milliseconds, Callback* cbk);
void clear_interval(u32 intervalId);

/* gpio handling */
[[nodiscard]] bool read_gpio(u32 port, u8 pin);
[[nodiscard]] u32 set_gpio(u32 port, u8 pin, Callback* cbk);
void clear_gpio(u32 gpio_id);
void send_gpio(u32 port, u8 pin, bool value);

/* ADC/DAC */
[[nodiscard]] f32 read_adc_f32(u32 port, u8 pin);
[[nodiscard]] f64 read_adc_f64(u32 port, u8 pin);
void send_dac_f32(u32 port, u8 pin, f32 value);
void send_dac_f64(u32 port, u8 pin, f64 value);

/* metrics */
void send_value_f32(u32 blockId, u8 inputId, f32 value);
void send_value_f64(u32 blockId, u8 inputId, f64 value);

/* common functions */
[[nodiscard]] f32 random_f32();
[[nodiscard]] f64 random_f64();

/* time functions */
[[nodiscard]] u64 get_time();

/* math functions */
[[nodiscard]] f32 sin_f32(f32 value);
[[nodiscard]] f64 sin_f64(f64 value);
[[nodiscard]] f32 cos_f32(f32 value);
[[nodiscard]] f64 cos_f64(f64 value);
[[nodiscard]] f32 tan_f32(f32 value);
[[nodiscard]] f64 tan_f64(f64 value);
[[nodiscard]] f32 asin_f32(f32 value);
[[nodiscard]] f64 asin_f64(f64 value);
[[nodiscard]] f32 acos_f32(f32 value);
[[nodiscard]] f64 acos_f64(f64 value);
[[nodiscard]] f32 atan_f32(f32 value);
[[nodiscard]] f64 atan_f64(f64 value);
[[nodiscard]] f32 exp_f32(f32 value);
[[nodiscard]] f64 exp_f64(f64 value);
[[nodiscard]] f32 log_f32(f32 value);
[[nodiscard]] f64 log_f64(f64 value);
[[nodiscard]] f32 log10_f32(f32 value);
[[nodiscard]] f64 log10_f64(f64 value);
[[nodiscard]] f32 pow_f32(f32 base, f32 exponent);
[[nodiscard]] f64 pow_f64(f64 base, f64 exponent);
[[nodiscard]] f32 sqrt_f32(f32 value);
[[nodiscard]] f64 sqrt_f64(f64 value);
[[nodiscard]] f32 ceil_f32(f32 value);
[[nodiscard]] f64 ceil_f64(f64 value);
[[nodiscard]] f32 floor_f32(f32 value);
[[nodiscard]] f64 floor_f64(f64 value);
}

[[nodiscard]] inline f32 nan_f32() {
  const u32 bits = 0x7fc00000u;
  f32 value;
  __builtin_memcpy(&value, &bits, sizeof(value));
  return value;
}

[[nodiscard]] inline f64 nan_f64() {
  const u64 bits = 0x7ff8000000000000ull;
  f64 value;
  __builtin_memcpy(&value, &bits, sizeof(value));
  return value;
}

[[nodiscard]] inline bool is_finite_f32(f32 value) {
  u32 bits = 0;
  __builtin_memcpy(&bits, &value, sizeof(bits));
  return (bits & 0x7f800000u) != 0x7f800000u;
}

[[nodiscard]] inline bool is_finite_f64(f64 value) {
  u64 bits = 0;
  __builtin_memcpy(&bits, &value, sizeof(bits));
  return (bits & 0x7ff0000000000000ull) != 0x7ff0000000000000ull;
}

template <typename T>
[[nodiscard]] T nan_of();

template <>
[[nodiscard]] inline f32 nan_of<f32>() {
  return nan_f32();
}

template <>
[[nodiscard]] inline f64 nan_of<f64>() {
  return nan_f64();
}

[[nodiscard]] inline bool is_finite(f32 value) { return is_finite_f32(value); }
[[nodiscard]] inline bool is_finite(f64 value) { return is_finite_f64(value); }

template <typename T>
[[nodiscard]] T random_of();

template <>
[[nodiscard]] inline f32 random_of<f32>() {
  return random_f32();
}

template <>
[[nodiscard]] inline f64 random_of<f64>() {
  return random_f64();
}
