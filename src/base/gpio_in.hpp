#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * GpioIn
 * @brief Pushes GPIO levels to the streams for each configured pin.
 * @image push.gpio_in.svg
 */
template <typename I, typename O>
class GpioIn : public NativeBlock<I, O> {
 public:
  /**
   * Value
   * @brief The numeric type carried by this block.
   * @image type.svg
   */
  using T = typename I::Value;

  explicit GpioIn(u32 blockId, u16 port, Array<u8> pins) : NativeBlock<I, O>(blockId), port_(port), pins_(move(pins)) {}

  static_assert(std::is_void_v<O>, "GPIO sources have no returned ports");

  O apply(I input) override {
    connected_ = static_cast<u8>(input.pins.size() < kMaxPins ? input.pins.size() : kMaxPins);
    for (u8 i = 0; i < connected_; ++i) {
      pinConsumers_[i] = move(input.pins[i]);
    }
    for (u32 i = 0; i < pins_.size() && i < kMaxPins; ++i) {
      slots_[i].parent = this;
      slots_[i].pinNumber = pins_[i];
      slots_[i].callback.slot = &slots_[i];
      handles_[i] = this->setGpio(port_, pins_[i], slots_[i].callback);
      handleCount_ = i + 1;
    }
    this->onClose(closeCb_);
  }

  [[nodiscard]] auto port() const { return port_; }
  [[nodiscard]] auto pins() const -> const Array<u8>& { return pins_; }

  static constexpr u8 kMaxPins = 8;

  /**
   * PinSlot
   * @brief Stores the callback and pin identity for one GPIO listener.
   * @image push.gpio_in.svg
   */
  struct PinSlot {
    GpioIn* parent{nullptr};
    u8 pinNumber{0};
    /**
     * Cbk
     * @brief Delivers a GPIO event to its owning block.
     * @image consumer.svg
     */
    struct Cbk final : public Callback {
      explicit Cbk(PinSlot* owner) : slot(owner) {}
      void operator()() override;
      PinSlot* slot;
    } callback;
    PinSlot() : callback(this) {}
  };

 private:
  [[nodiscard]] auto searchPin(u8 pin) const -> u32 {
    for (u32 i = 0; i < pins_.size(); ++i) {
      if (pins_[i] == pin) {
        return i;
      }
    }
    return pins_.size();
  }

  void emitPin(u8 pinNumber) const {
    const auto idx = searchPin(pinNumber);
    if (idx < connected_) {
      this->pushTo(pinConsumers_[idx], read_gpio(port_, pinNumber) ? T{1} : T{0});
    }
  }

  void handleClose() {
    for (u32 i = 0; i < handleCount_; ++i) {
      this->clearGpio(handles_[i]);
    }
  }

  u16 port_;
  Array<u8> pins_;
  Vectorized<Consumer<T>> pinConsumers_[kMaxPins]{};
  PinSlot slots_[kMaxPins]{};
  u32 handles_[kMaxPins]{};
  u32 handleCount_{0};
  u8 connected_{0};
  MemberCallback<GpioIn<I, O>, &GpioIn<I, O>::handleClose> closeCb_{this};
};

template <typename I, typename O>
void GpioIn<I, O>::PinSlot::Cbk::operator()() {
  slot->parent->emitPin(slot->pinNumber);
}

}  // namespace push
