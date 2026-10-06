#pragma once

#include <base/native_block.hpp>
#include <utility>
#include <vector>

namespace push {

/**
 * GpioIn
 * @brief Pushes GPIO levels to the streams for each configured pin.
 * @image push.gpio_in.svg
 */
template <typename I> class GpioIn : public NativeBlock<I, void> {
public:
  using T = I::Value;

  /**
   * GpioIn
   * @param port Port
   *   Hardware GPIO port identifier to monitor.
   *   @icon port.svg
   *   @control number
   *   @min 0
   *   @max 65535
   *   @step 1
   * @param pins Pins
   *   GPIO pin indices to listen to for events.
   *   @icon push.gpio_in.svg
   *   @control text
   */
  explicit GpioIn(const u32       blockId,
                  const u16       port = 0,
                  std::vector<u8> pins = {0})
      : NativeBlock<I,
                    void>(blockId),
        port(port),
        pins(std::move(pins)) {}

  void apply(I input) override {
    connected = static_cast<u8>(input.pins.size() < kMaxPins ? input.pins.size() : kMaxPins);
    for (u8 i = 0; i < connected; ++i) {
      pinConsumers[i] = std::move(input.pins[i]);
    }
    for (u32 i = 0; i < pins.size() && i < kMaxPins; ++i) {
      slots[i].parent = this;
      slots[i].pinNumber = pins[i];
      slots[i].callback.slot = &slots[i];
      handles[i] = this->setGpio(port, pins[i], slots[i].callback);
      handleCount = i + 1;
    }
    this->onClose(closeCb);
  }

  const u16             port;
  const std::vector<u8> pins;

  static constexpr u8 kMaxPins = 8;

  /**
   * PinSlot
   * @brief Stores the callback and pin identity for one GPIO listener.
   * @image push.gpio_in.svg
   */
  struct PinSlot {
    GpioIn *parent{nullptr};
    u8      pinNumber{0};
    /**
     * Cbk
     * @brief Delivers a GPIO event to its owning block.
     * @image consumer.svg
     */
    struct Cbk final : public Callback {
      explicit Cbk(PinSlot *const owner) : slot(owner) {}
      void     operator()() override;
      PinSlot *slot;
    } callback;
    PinSlot() : callback(this) {}
  };

private:
  [[nodiscard]]
  auto searchPin(const u8 pin) const -> u32 {
    for (u32 i = 0; i < pins.size(); ++i) {
      if (pins[i] == pin) {
        return i;
      }
    }
    return pins.size();
  }

  void emitPin(const u8 pinNumber) const {
    const auto idx = searchPin(pinNumber);
    if (idx < connected) {
      this->pushTo(pinConsumers[idx], read_gpio(port, pinNumber) ? T{1} : T{0});
    }
  }

  void handleClose() {
    for (u32 i = 0; i < handleCount; ++i) {
      this->clearGpio(handles[i]);
    }
  }

  VectorizedInput<Consumer<T>>         pinConsumers[kMaxPins]{};
  PinSlot                              slots[kMaxPins]{};
  u32                                  handles[kMaxPins]{};
  u32                                  handleCount{0};
  u8                                   connected{0};
  MemberConsumer<&GpioIn::handleClose> closeCb{this};
};

template <typename I> void GpioIn<I>::PinSlot::Cbk::operator()() { slot->parent->emitPin(slot->pinNumber); }

} // namespace push
