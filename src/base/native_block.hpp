#pragma once

#include <core/array.hpp>
#include <core/block.hpp>
#include <core/callback.hpp>
#include <core/hal.hpp>
#include <core/move.hpp>

/**
 * NativeBlock
 * @brief A typed block with access to native runtime services.
 * @image block.svg
 */
template <typename I, typename O>
class NativeBlock : public Block<I, O> {
 public:
  using Block<I, O>::Block;
  ~NativeBlock() override = default;

 protected:
  /**
   * ClearIntervalCallback
   * @brief Clears a runtime interval when invoked.
   * @image consumer.svg
   */
  class ClearIntervalCallback final : public Callback {
   public:
    explicit ClearIntervalCallback(const u32 timer) : timer_(timer) {}
    void operator()() override { clearInterval(timer_); }

   private:
    u32 timer_;
  };

  /**
   * ClearGpioHandlesCallback
   * @brief Clears the owned GPIO listener handles when invoked.
   * @image consumer.svg
   */
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

  auto setInterval(const auto milliseconds,
                   auto&      callback) {
    return set_interval(milliseconds, &callback);
  }

  static void clearInterval(const auto intervalId) { clear_interval(intervalId); }

  auto setGpio(const auto port,
               const auto pin,
               auto&      callback) {
    return set_gpio(port, pin, &callback);
  }

  static void clearGpio(const auto gpioId) { clear_gpio(gpioId); }

  void armInterval(const auto milliseconds,
                   auto&      tick,
                   auto&      closeSlot) {
    auto timer = setInterval(milliseconds, tick);
    closeSlot.emplace(timer);
    onClose(*closeSlot);
  }

  void sendValue(const u8  channel,
                 const f32 value) const {
    send_value_f32(this->blockId, channel, value);
  }
  void sendValue(const u8  channel,
                 const f64 value) const {
    send_value_f64(this->blockId, channel, value);
  }

  static void pushTo(const auto& sinks,
                     const auto  value) {
    for (auto* sink : sinks) {
      if (sink) {
        (*sink)(value);
      }
    }
  }

  template <typename T,
            typename Item>
  static auto pointersOf(Array<Item>& items) -> Vectorized<Consumer<T>> {
    auto result = Vectorized<Consumer<T>>{};
    result.reserve(items.size());
    for (u32 i = 0; i < items.size(); ++i) {
      result.push_back(&items[i]);
    }
    return result;
  }
};
