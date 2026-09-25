#pragma once

#include <core/array.hpp>
#include <core/hal.hpp>


void register_gpio_block(u32 blockId, u16 port, const Array<u8>& pins);

extern "C" void mount();



namespace wasm {

constexpr u32 kCap = 64;
constexpr u32 kMaxPins = 8;

inline void invoke(Callback* callback) {
  if (callback) {
    (*callback)();
  }
}

class WasmHost {
 public:
  static WasmHost instanceHost;
  static WasmHost& instance() { return instanceHost; }

  void onStart(Callback* callback) { pushCallback(start_, startCount_, callback); }
  void onClose(Callback* callback) { pushCallback(close_, closeCount_, callback); }
  void onStop(Callback* callback) { pushCallback(stop_, stopCount_, callback); }

  u32 setInterval(u32 milliseconds, Callback* callback) {
    if (intervalCount_ >= kCap) {
      return 0;
    }
    const u32 id = nextIntervalId_++;
    intervals_[intervalCount_++] = Interval{id, milliseconds, callback, true};
    return id;
  }

  void clearInterval(u32 intervalId) {
    for (u32 i = 0; i < intervalCount_; ++i) {
      if (intervals_[i].id == intervalId) {
        intervals_[i].active = false;
      }
    }
  }

  bool readGpio(u32 port, u8 pin) const {
    for (u32 i = 0; i < kCap; ++i) {
      if (gpioValues_[i].used && gpioValues_[i].port == port && gpioValues_[i].pin == pin) {
        return gpioValues_[i].value;
      }
    }
    return false;
  }

  u32 setGpio(u32 port, u8 pin, Callback* callback) {
    if (gpioCount_ >= kCap) {
      return 0;
    }
    const u32 id = nextGpioId_++;
    gpio_[gpioCount_++] = GpioListener{id, port, pin, callback, true};
    return id;
  }

  void clearGpio(u32 gpioId) {
    for (u32 i = 0; i < gpioCount_; ++i) {
      if (gpio_[i].id == gpioId) {
        gpio_[i].active = false;
      }
    }
  }

  void sendGpio(u32 port, u8 pin, bool value) {
    setGpioValue(port, pin, value);
    fireGpio(port, pin);
  }

  void sendF32(u32 blockId, u8 inputId, f32 value) {
    setF32(blockId, inputId, value);
    ++pinWriteCount_;
  }

  void sendF64(u32 blockId, u8 inputId, f64 value) { setF64(blockId, inputId, value); }

  f32 randomF32() const { return random_; }
  f64 randomF64() const { return static_cast<f64>(random_); }
  u64 now() const { return now_; }

  void start() {
    if (!built_) {
      built_ = true;
      mount();
    }
    if (started_) {
      return;
    }
    started_ = true;
    const u32 count = startCount_;
    for (u32 i = 0; i < count; ++i) {
      invoke(start_[i]);
    }
  }

  void close() {
    const u32 count = closeCount_;
    for (u32 i = 0; i < count; ++i) {
      invoke(close_[i]);
    }
  }

  void tick() {
    const u32 count = intervalCount_;
    for (u32 i = 0; i < count; ++i) {
      if (intervals_[i].active) {
        invoke(intervals_[i].callback);
      }
    }
  }

  void setNow(u64 milliseconds) { now_ = milliseconds; }
  void setRandom(f32 value) { random_ = value; }

  void emitGpioIn(u32 blockId, u32 pinIndex, bool value) {
    for (u32 i = 0; i < gpioBlockCount_; ++i) {
      const GpioBlock& block = gpioBlocks_[i];
      if (block.blockId == blockId && pinIndex < block.pinCount) {
        sendGpio(block.port, block.pins[pinIndex], value);
        return;
      }
    }
  }

  void registerGpioBlock(u32 blockId, u16 port, const Array<u8>& pins) {
    if (gpioBlockCount_ >= kCap) {
      return;
    }
    GpioBlock& block = gpioBlocks_[gpioBlockCount_++];
    block.blockId = blockId;
    block.port = port;
    block.pinCount = pins.size() < kMaxPins ? static_cast<u8>(pins.size()) : static_cast<u8>(kMaxPins);
    for (u8 i = 0; i < block.pinCount; ++i) {
      block.pins[i] = pins[i];
    }
  }

  bool hasF32(u32 blockId, u8 channel) const {
    for (u32 i = 0; i < kCap; ++i) {
      if (valuesF32_[i].used && valuesF32_[i].blockId == blockId && valuesF32_[i].channel == channel) {
        return true;
      }
    }
    return false;
  }

  f32 lastF32(u32 blockId, u8 channel) const {
    for (u32 i = 0; i < kCap; ++i) {
      if (valuesF32_[i].used && valuesF32_[i].blockId == blockId && valuesF32_[i].channel == channel) {
        return valuesF32_[i].value;
      }
    }
    return nan_f32();
  }

  u32 activeIntervalCount() const {
    u32 count = 0;
    for (u32 i = 0; i < intervalCount_; ++i) {
      if (intervals_[i].active) {
        ++count;
      }
    }
    return count;
  }

  u32 activeGpioCount() const {
    u32 count = 0;
    for (u32 i = 0; i < gpioCount_; ++i) {
      if (gpio_[i].active) {
        ++count;
      }
    }
    return count;
  }

  u32 intervalPeriodAt(u32 index) const {
    u32 seen = 0;
    for (u32 i = 0; i < intervalCount_; ++i) {
      if (!intervals_[i].active) {
        continue;
      }
      if (seen == index) {
        return intervals_[i].period;
      }
      ++seen;
    }
    return 0;
  }

  u32 pinWriteCount() const { return pinWriteCount_; }

  void clearPins() {
    for (u32 i = 0; i < kCap; ++i) {
      valuesF32_[i].used = false;
      valuesF64_[i].used = false;
    }
    pinWriteCount_ = 0;
  }

 private:
  struct Interval {
    u32 id;
    u32 period;
    Callback* callback;
    bool active;
  };

  struct GpioListener {
    u32 id;
    u32 port;
    u8 pin;
    Callback* callback;
    bool active;
  };

  struct GpioBlock {
    u32 blockId;
    u16 port;
    u8 pinCount;
    u8 pins[kMaxPins];
  };

  struct GpioValue {
    u32 port;
    u8 pin;
    bool value;
    bool used;
  };

  struct F32Value {
    u32 blockId;
    u8 channel;
    f32 value;
    bool used;
  };

  struct F64Value {
    u32 blockId;
    u8 channel;
    f64 value;
    bool used;
  };

  static void pushCallback(Callback** list, u32& count, Callback* callback) {
    if (count < kCap) {
      list[count++] = callback;
    }
  }

  void setGpioValue(u32 port, u8 pin, bool value) {
    for (u32 i = 0; i < kCap; ++i) {
      if (gpioValues_[i].used && gpioValues_[i].port == port && gpioValues_[i].pin == pin) {
        gpioValues_[i].value = value;
        return;
      }
    }
    for (u32 i = 0; i < kCap; ++i) {
      if (!gpioValues_[i].used) {
        gpioValues_[i] = GpioValue{port, pin, value, true};
        return;
      }
    }
  }

  void setF32(u32 blockId, u8 channel, f32 value) {
    for (u32 i = 0; i < kCap; ++i) {
      if (valuesF32_[i].used && valuesF32_[i].blockId == blockId && valuesF32_[i].channel == channel) {
        valuesF32_[i].value = value;
        return;
      }
    }
    for (u32 i = 0; i < kCap; ++i) {
      if (!valuesF32_[i].used) {
        valuesF32_[i] = F32Value{blockId, channel, value, true};
        return;
      }
    }
  }

  void setF64(u32 blockId, u8 channel, f64 value) {
    for (u32 i = 0; i < kCap; ++i) {
      if (valuesF64_[i].used && valuesF64_[i].blockId == blockId && valuesF64_[i].channel == channel) {
        valuesF64_[i].value = value;
        return;
      }
    }
    for (u32 i = 0; i < kCap; ++i) {
      if (!valuesF64_[i].used) {
        valuesF64_[i] = F64Value{blockId, channel, value, true};
        return;
      }
    }
  }

  void fireGpio(u32 port, u8 pin) {
    const u32 count = gpioCount_;
    for (u32 i = 0; i < count; ++i) {
      const GpioListener& listener = gpio_[i];
      if (listener.active && listener.port == port && listener.pin == pin) {
        invoke(listener.callback);
      }
    }
  }

  Callback* start_[kCap]{};
  u32 startCount_{0};
  Callback* close_[kCap]{};
  u32 closeCount_{0};
  Callback* stop_[kCap]{};
  u32 stopCount_{0};
  Interval intervals_[kCap]{};
  u32 intervalCount_{0};
  GpioListener gpio_[kCap]{};
  u32 gpioCount_{0};
  GpioBlock gpioBlocks_[kCap]{};
  u32 gpioBlockCount_{0};
  GpioValue gpioValues_[kCap]{};
  F32Value valuesF32_[kCap]{};
  F64Value valuesF64_[kCap]{};
  u32 nextIntervalId_{1};
  u32 nextGpioId_{1};
  u32 pinWriteCount_{0};
  u64 now_{0};
  f32 random_{0};
  bool built_{false};
  bool started_{false};
};

inline WasmHost WasmHost::instanceHost{};

}  // namespace wasm

inline void register_gpio_block(u32 blockId, u16 port, const Array<u8>& pins) {
  wasm::WasmHost::instance().registerGpioBlock(blockId, port, pins);
}

extern "C" {

inline void on_close(Callback* cbk) { wasm::WasmHost::instance().onClose(cbk); }
inline void on_start(Callback* cbk) { wasm::WasmHost::instance().onStart(cbk); }
inline void on_stop(Callback* cbk) { wasm::WasmHost::instance().onStop(cbk); }

inline u32 set_interval(u32 milliseconds, Callback* cbk) { return wasm::WasmHost::instance().setInterval(milliseconds, cbk); }
inline void clear_interval(u32 intervalId) { wasm::WasmHost::instance().clearInterval(intervalId); }

inline bool read_gpio(u32 port, u8 pin) { return wasm::WasmHost::instance().readGpio(port, pin); }
inline u32 set_gpio(u32 port, u8 pin, Callback* cbk) { return wasm::WasmHost::instance().setGpio(port, pin, cbk); }
inline void clear_gpio(u32 gpio_id) { wasm::WasmHost::instance().clearGpio(gpio_id); }
inline void send_gpio(u32 port, u8 pin, bool value) { wasm::WasmHost::instance().sendGpio(port, pin, value); }

inline f32 read_adc_f32(u32, u8) { return 0; }
inline f64 read_adc_f64(u32, u8) { return 0; }
inline void send_dac_f32(u32, u8, f32) {}
inline void send_dac_f64(u32, u8, f64) {}

void host_sendPinF32(u32 blockId, u8 pin, f32 value);

inline void send_value_f32(u32 blockId, u8 inputId, f32 value) {
  wasm::WasmHost::instance().sendF32(blockId, inputId, value);
  host_sendPinF32(blockId, inputId, value);
}
inline void send_value_f64(u32 blockId, u8 inputId, f64 value) { wasm::WasmHost::instance().sendF64(blockId, inputId, value); }

inline f32 random_f32() { return wasm::WasmHost::instance().randomF32(); }
inline f64 random_f64() { return wasm::WasmHost::instance().randomF64(); }

inline u64 get_time() { return wasm::WasmHost::instance().now(); }

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

inline void start() { wasm::WasmHost::instance().start(); }
inline void tick() { wasm::WasmHost::instance().tick(); }
inline void tickThenObserve() { wasm::WasmHost::instance().tick(); }
inline void close() { wasm::WasmHost::instance().close(); }
inline void setNow(u32 ms) { wasm::WasmHost::instance().setNow(ms); }
inline void setRandom(f32 value) { wasm::WasmHost::instance().setRandom(value); }
inline void emitGpio(u32 port, u32 pin, u32 value) { wasm::WasmHost::instance().sendGpio(port, static_cast<u8>(pin), value != 0); }
inline void emitGpioIn(u32 blockId, u32 pinIndex, u32 value) { wasm::WasmHost::instance().emitGpioIn(blockId, pinIndex, value != 0); }
inline f32 lastPin(u32 blockId, u32 pin) { return wasm::WasmHost::instance().lastF32(blockId, static_cast<u8>(pin)); }
inline u32 hasPin(u32 blockId, u32 pin) { return wasm::WasmHost::instance().hasF32(blockId, static_cast<u8>(pin)) ? 1 : 0; }
inline u32 pinWriteCount() { return wasm::WasmHost::instance().pinWriteCount(); }
inline u32 activeIntervalCount() { return wasm::WasmHost::instance().activeIntervalCount(); }
inline u32 intervalPeriodAt(u32 index) { return wasm::WasmHost::instance().intervalPeriodAt(index); }
inline u32 activeGpioListenerCount() { return wasm::WasmHost::instance().activeGpioCount(); }
inline void clearPins() { wasm::WasmHost::instance().clearPins(); }
}

