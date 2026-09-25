#pragma once

#include <base/pss.hpp>
#include <core/block.hpp>
#include <core/callback.hpp>
#include <core/hal.hpp>
#include <core/maybe.hpp>
#include <core/move.hpp>

class NativeBlock : public Block {
 public:
  using Block::Block;
  ~NativeBlock() override = default;

 protected:
  class ClearIntervalCallback final : public Callback {
   public:
    explicit ClearIntervalCallback(u32 timer) : timer_(timer) {}
    void operator()() override { clearInterval(timer_); }

   private:
    u32 timer_;
  };

  class ClearGpioHandlesCallback final : public Callback {
   public:
    explicit ClearGpioHandlesCallback(Array<u32> handles) : handles_(move(handles)) {}
    void operator()() override {
      for (auto handle : handles_) {
        clearGpio(handle);
      }
    }

   private:
    Array<u32> handles_;
  };

  void onStart(auto& callback) { on_start(&callback); }

  void onClose(auto& callback) { on_close(&callback); }

  [[nodiscard]] auto setInterval(auto milliseconds, auto& callback) { return set_interval(milliseconds, &callback); }

  static void clearInterval(auto intervalId) { clear_interval(intervalId); }

  [[nodiscard]] auto setGpio(auto port, auto pin, auto& callback) { return set_gpio(port, pin, &callback); }

  static void clearGpio(auto gpioId) { clear_gpio(gpioId); }

  void armInterval(auto milliseconds, auto& tick, auto& closeSlot) {
    auto timer = setInterval(milliseconds, tick);
    closeSlot.emplace(timer);
    onClose(*closeSlot);
  }

  void sendValue(u8 channel, f32 value) const { send_value_f32(blockId, channel, value); }
  void sendValue(u8 channel, f64 value) const { send_value_f64(blockId, channel, value); }

  static void pushTo(const auto& sinks, auto value) {
    for (auto* sink : sinks) {
      if (sink) {
        (*sink)(value);
      }
    }
  }

  template <typename T, typename Item>
  [[nodiscard]] static auto pointersOf(Array<Item>& items) -> Vectorized<Pss<T>> {
    auto result = Vectorized<Pss<T>>{};
    result.reserve(items.size());
    for (u32 i = 0; i < items.size(); ++i) {
      result.push_back(&items[i]);
    }
    return result;
  }
};
