#pragma once

// The bare Wasm sysroot has no libm. Hosts supply these math imports (for a
// browser, Math.sin/Math.cos and floating-point remainder). Native builtins use
// the host toolchain's math library.
#if defined(__wasm__)
extern "C" {
__attribute__((import_module("env"),
               import_name("sin"))) double
core_sin(double);
__attribute__((import_module("env"),
               import_name("cos"))) double
core_cos(double);
__attribute__((import_module("env"),
               import_name("fmod"))) double
core_fmod(double,
          double);
}
#endif

namespace core {
template <typename T>
inline constexpr T                pi_v = static_cast<T>(3.141592653589793238462643383279502884L);
template <typename T> constexpr T quiet_nan() { return static_cast<T>(__builtin_nan("")); }
template <typename T> bool        isfinite(T value) { return __builtin_isfinite(value); }
template <typename T> T           sin(T value) {
#if defined(__wasm__)
  return static_cast<T>(core_sin(static_cast<double>(value)));
#else
  return static_cast<T>(__builtin_sin(static_cast<double>(value)));
#endif
}
template <typename T> T cos(T value) {
#if defined(__wasm__)
  return static_cast<T>(core_cos(static_cast<double>(value)));
#else
  return static_cast<T>(__builtin_cos(static_cast<double>(value)));
#endif
}
template <typename T>
T fmod(T value,
       T divisor) {
#if defined(__wasm__)
  return static_cast<T>(core_fmod(static_cast<double>(value), static_cast<double>(divisor)));
#else
  return static_cast<T>(__builtin_fmod(static_cast<double>(value), static_cast<double>(divisor)));
#endif
}
} // namespace core
