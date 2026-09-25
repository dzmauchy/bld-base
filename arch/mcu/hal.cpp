#include <bld.hpp>

// Bare-metal host for the C HAL. Firmware calls bld_mcu_start() once the
// blocks are constructed, bld_mcu_timer_isr() from the hardware timer, and
// bld_mcu_close() on shutdown. Pin access goes through the weak board hooks.

namespace {

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

CallbackSlot starts[kCap]{};
CallbackSlot closes[kCap]{};
CallbackSlot stops[kCap]{};
Interval intervals[kCap]{};
GpioListener gpio[kCap]{};
GpioLevel levels[kCap]{};
u32 nextIntervalId = 1;
u32 nextGpioId = 1;
u64 nowMs = 0;
u32 randomState = 1;

void invoke(Callback* callback) {
  if (callback) {
    (*callback)();
  }
}

void pushCallback(CallbackSlot* slots, Callback* callback) {
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

void fireList(CallbackSlot* slots) {
  for (u32 i = 0; i < kCap; ++i) {
    if (slots[i].used) {
      invoke(slots[i].callback);
    }
  }
}

bool findLevel(u32 port, u8 pin, u32& index) {
  for (u32 i = 0; i < kCap; ++i) {
    if (levels[i].used && levels[i].port == port && levels[i].pin == pin) {
      index = i;
      return true;
    }
  }
  return false;
}

void storeLevel(u32 port, u8 pin, bool value) {
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

void fireGpio(u32 port, u8 pin) {
  for (u32 i = 0; i < kCap; ++i) {
    if (gpio[i].active && gpio[i].port == port && gpio[i].pin == pin) {
      invoke(gpio[i].callback);
    }
  }
}

}  // namespace

extern "C" __attribute__((weak)) bool bld_mcu_read_pin(u32 port, u8 pin) {
  u32 index = 0;
  if (findLevel(port, pin, index)) {
    return levels[index].value;
  }
  return false;
}

extern "C" __attribute__((weak)) void bld_mcu_write_pin(u32 port, u8 pin, bool value) { storeLevel(port, pin, value); }

extern "C" void bld_mcu_start() { fireList(starts); }

extern "C" void bld_mcu_stop() { fireList(stops); }

extern "C" void bld_mcu_close() { fireList(closes); }

extern "C" void bld_mcu_timer_isr() {
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

void on_close(Callback* cbk) { pushCallback(closes, cbk); }
void on_start(Callback* cbk) { pushCallback(starts, cbk); }
void on_stop(Callback* cbk) { pushCallback(stops, cbk); }

u32 set_interval(u32 milliseconds, Callback* cbk) {
  for (u32 i = 0; i < kCap; ++i) {
    if (!intervals[i].active) {
      const u32 id = nextIntervalId++;
      intervals[i] = Interval{id, milliseconds, 0, cbk, true};
      return id;
    }
  }
  return 0;
}

void clear_interval(u32 intervalId) {
  for (u32 i = 0; i < kCap; ++i) {
    if (intervals[i].id == intervalId) {
      intervals[i].active = false;
    }
  }
}

bool read_gpio(u32 port, u8 pin) { return bld_mcu_read_pin(port, pin); }

u32 set_gpio(u32 port, u8 pin, Callback* cbk) {
  for (u32 i = 0; i < kCap; ++i) {
    if (!gpio[i].active) {
      const u32 id = nextGpioId++;
      gpio[i] = GpioListener{id, port, pin, cbk, true};
      return id;
    }
  }
  return 0;
}

void clear_gpio(u32 gpio_id) {
  for (u32 i = 0; i < kCap; ++i) {
    if (gpio[i].id == gpio_id) {
      gpio[i].active = false;
    }
  }
}

void send_gpio(u32 port, u8 pin, bool value) {
  bld_mcu_write_pin(port, pin, value);
  fireGpio(port, pin);
}

f32 read_adc_f32(u32, u8) { return 0; }
f64 read_adc_f64(u32, u8) { return 0; }
void send_dac_f32(u32, u8, f32) {}
void send_dac_f64(u32, u8, f64) {}

void send_value_f32(u32, u8, f32) {}
void send_value_f64(u32, u8, f64) {}

f32 random_f32() {
  randomState = randomState * 1664525u + 1013904223u;
  return static_cast<f32>(randomState >> 8) * (1.f / 16777216.f);
}

f64 random_f64() { return static_cast<f64>(random_f32()); }

u64 get_time() { return nowMs; }

f32 sin_f32(f32 value) { return __builtin_sinf(value); }
f64 sin_f64(f64 value) { return __builtin_sin(value); }
f32 cos_f32(f32 value) { return __builtin_cosf(value); }
f64 cos_f64(f64 value) { return __builtin_cos(value); }
f32 tan_f32(f32 value) { return __builtin_sinf(value) / __builtin_cosf(value); }
f64 tan_f64(f64 value) { return __builtin_sin(value) / __builtin_cos(value); }
f32 asin_f32(f32) { return 0; }
f64 asin_f64(f64) { return 0; }
f32 acos_f32(f32) { return 0; }
f64 acos_f64(f64) { return 0; }
f32 atan_f32(f32) { return 0; }
f64 atan_f64(f64) { return 0; }
f32 exp_f32(f32) { return 0; }
f64 exp_f64(f64) { return 0; }
f32 log_f32(f32) { return 0; }
f64 log_f64(f64) { return 0; }
f32 log10_f32(f32) { return 0; }
f64 log10_f64(f64) { return 0; }
f32 pow_f32(f32, f32) { return 0; }
f64 pow_f64(f64, f64) { return 0; }
f32 sqrt_f32(f32 value) { return __builtin_sqrtf(value); }
f64 sqrt_f64(f64 value) { return __builtin_sqrt(value); }
f32 ceil_f32(f32 value) { return __builtin_ceilf(value); }
f64 ceil_f64(f64 value) { return __builtin_ceil(value); }
f32 floor_f32(f32 value) { return __builtin_floorf(value); }
f64 floor_f64(f64 value) { return __builtin_floor(value); }

}
