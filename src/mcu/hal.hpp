#pragma once

#include <core/hal.hpp>

// Bare-metal host for the C HAL. Firmware calls bld_mcu_start() once the
// blocks are constructed, bld_mcu_timer_isr() from the hardware timer, and
// bld_mcu_close() on shutdown. Pin access goes through the weak board hooks.

namespace mcu {

constexpr u32 kCap = 32;

struct CallbackSlot {
  Callback* callback{nullptr};
  bool used{false};
};

struct Interval {
  u32 id{0};
  u32 period{0};
  u32 elapsed{0};
  Callback* callback{nullptr};
  bool active{false};
};

struct GpioListener {
  u32 id{0};
  u32 port{0};
  u8 pin{0};
  Callback* callback{nullptr};
  bool active{false};
};

struct GpioLevel {
  u32 port{0};
  u8 pin{0};
  bool value{false};
  bool used{false};
};

inline CallbackSlot starts[kCap]{};
inline CallbackSlot closes[kCap]{};
inline CallbackSlot stops[kCap]{};
inline Interval intervals[kCap]{};
inline GpioListener gpio[kCap]{};
inline GpioLevel levels[kCap]{};
inline u32 nextIntervalId = 1;
inline u32 nextGpioId = 1;
inline u64 nowMs = 0;
inline u32 randomState = 1;

inline void invoke(Callback* callback) {
  if (callback) {
    (*callback)();
  }
}

inline void pushCallback(CallbackSlot* slots, Callback* callback) {
  if (!callback) {
    return;
  }
  for (u32 i = 0; i < kCap; ++i) {
    if (!slots[i].used) {
      slots[i] = CallbackSlot{callback, true};
      return;
    }
  }
}

inline void fireList(CallbackSlot* slots) {
  for (u32 i = 0; i < kCap; ++i) {
    if (slots[i].used) {
      invoke(slots[i].callback);
    }
  }
}

inline bool findLevel(u32 port, u8 pin, u32& index) {
  for (u32 i = 0; i < kCap; ++i) {
    if (levels[i].used && levels[i].port == port && levels[i].pin == pin) {
      index = i;
      return true;
    }
  }
  return false;
}

inline void storeLevel(u32 port, u8 pin, bool value) {
  u32 index = 0;
  if (findLevel(port, pin, index)) {
    levels[index].value = value;
    return;
  }
  for (u32 i = 0; i < kCap; ++i) {
    if (!levels[i].used) {
      levels[i] = GpioLevel{port, pin, value, true};
      return;
    }
  }
}

inline void fireGpio(u32 port, u8 pin) {
  for (u32 i = 0; i < kCap; ++i) {
    if (gpio[i].active && gpio[i].port == port && gpio[i].pin == pin) {
      invoke(gpio[i].callback);
    }
  }
}

extern "C" inline __attribute__((weak)) bool bld_mcu_read_pin(u32 port, u8 pin) {
  u32 index = 0;
  if (findLevel(port, pin, index)) {
    return levels[index].value;
  }
  return false;
}

extern "C" inline __attribute__((weak)) void bld_mcu_write_pin(u32 port, u8 pin, bool value) { storeLevel(port, pin, value); }

extern "C" inline void bld_mcu_start() { fireList(starts); }

extern "C" inline void bld_mcu_stop() { fireList(stops); }

extern "C" inline void bld_mcu_close() { fireList(closes); }

extern "C" inline void bld_mcu_timer_isr() {
  ++nowMs;
  for (u32 i = 0; i < kCap; ++i) {
    if (!intervals[i].active || intervals[i].period == 0) {
      continue;
    }
    ++intervals[i].elapsed;
    if (intervals[i].elapsed >= intervals[i].period) {
      intervals[i].elapsed = 0;
      invoke(intervals[i].callback);
    }
  }
}

extern "C" {

inline void on_close(Callback* cbk) { pushCallback(closes, cbk); }
inline void on_start(Callback* cbk) { pushCallback(starts, cbk); }
inline void on_stop(Callback* cbk) { pushCallback(stops, cbk); }

inline u32 set_interval(u32 milliseconds, Callback* cbk) {
  for (u32 i = 0; i < kCap; ++i) {
    if (!intervals[i].active) {
      const u32 id = nextIntervalId++;
      intervals[i] = Interval{id, milliseconds, 0, cbk, true};
      return id;
    }
  }
  return 0;
}

inline void clear_interval(u32 intervalId) {
  for (u32 i = 0; i < kCap; ++i) {
    if (intervals[i].id == intervalId) {
      intervals[i].active = false;
    }
  }
}

inline bool read_gpio(u32 port, u8 pin) { return bld_mcu_read_pin(port, pin); }

inline u32 set_gpio(u32 port, u8 pin, Callback* cbk) {
  for (u32 i = 0; i < kCap; ++i) {
    if (!gpio[i].active) {
      const u32 id = nextGpioId++;
      gpio[i] = GpioListener{id, port, pin, cbk, true};
      return id;
    }
  }
  return 0;
}

inline void clear_gpio(u32 gpio_id) {
  for (u32 i = 0; i < kCap; ++i) {
    if (gpio[i].id == gpio_id) {
      gpio[i].active = false;
    }
  }
}

inline void send_gpio(u32 port, u8 pin, bool value) {
  bld_mcu_write_pin(port, pin, value);
  fireGpio(port, pin);
}

inline f32 read_adc_f32(u32, u8) { return 0; }
inline f64 read_adc_f64(u32, u8) { return 0; }
inline void send_dac_f32(u32, u8, f32) {}
inline void send_dac_f64(u32, u8, f64) {}

inline void send_value_f32(u32, u8, f32) {}
inline void send_value_f64(u32, u8, f64) {}

inline f32 random_f32() {
  randomState = randomState * 1664525u + 1013904223u;
  return static_cast<f32>(randomState >> 8) * (1.f / 16777216.f);
}

inline f64 random_f64() { return static_cast<f64>(random_f32()); }

inline u64 get_time() { return nowMs; }

inline f32 sin_f32(f32 value) { return __builtin_sinf(value); }
inline f64 sin_f64(f64 value) { return __builtin_sin(value); }
inline f32 cos_f32(f32 value) { return __builtin_cosf(value); }
inline f64 cos_f64(f64 value) { return __builtin_cos(value); }
inline f32 tan_f32(f32 value) { return __builtin_sinf(value) / __builtin_cosf(value); }
inline f64 tan_f64(f64 value) { return __builtin_sin(value) / __builtin_cos(value); }
inline f32 asin_f32(f32) { return 0; }
inline f64 asin_f64(f64) { return 0; }
inline f32 acos_f32(f32) { return 0; }
inline f64 acos_f64(f64) { return 0; }
inline f32 atan_f32(f32) { return 0; }
inline f64 atan_f64(f64) { return 0; }
inline f32 exp_f32(f32) { return 0; }
inline f64 exp_f64(f64) { return 0; }
inline f32 log_f32(f32) { return 0; }
inline f64 log_f64(f64) { return 0; }
inline f32 log10_f32(f32) { return 0; }
inline f64 log10_f64(f64) { return 0; }
inline f32 pow_f32(f32, f32) { return 0; }
inline f64 pow_f64(f64, f64) { return 0; }
inline f32 sqrt_f32(f32 value) { return __builtin_sqrtf(value); }
inline f64 sqrt_f64(f64 value) { return __builtin_sqrt(value); }
inline f32 ceil_f32(f32 value) { return __builtin_ceilf(value); }
inline f64 ceil_f64(f64 value) { return __builtin_ceil(value); }
inline f32 floor_f32(f32 value) { return __builtin_floorf(value); }
inline f64 floor_f64(f64 value) { return __builtin_floor(value); }

}

}  // namespace mcu
