#pragma once

#include <base/native_block.hpp>
#include <core/move.hpp>

namespace push {

template <typename T>
class GpioIn : public NativeBlock {
 public:
  explicit GpioIn(u32 blockId, u16 port, Array<u8> pins) : NativeBlock(blockId), port_(port), pins_(move(pins)) {}

  void connectPin(u8 pinIndex, Vectorized<Pss<T>> sinks) {
    if (pinIndex >= kMaxPins) {
      return;
    }
    pinConsumers_[pinIndex] = move(sinks);
    if (pinIndex + 1 > connected_) {
      connected_ = static_cast<u8>(pinIndex + 1);
    }
  }

  void apply() {
    for (u32 i = 0; i < pins_.size() && i < kMaxPins; ++i) {
      slots_[i].parent = this;
      slots_[i].pinNumber = pins_[i];
      slots_[i].callback.slot = &slots_[i];
      handles_[i] = setGpio(port_, pins_[i], slots_[i].callback);
      handleCount_ = i + 1;
    }
    onClose(closeCb_);
  }

  [[nodiscard]] auto port() const { return port_; }
  [[nodiscard]] auto pins() const -> const Array<u8>& { return pins_; }

  static constexpr u8 kMaxPins = 8;

  struct PinSlot {
    GpioIn* parent{nullptr};
    u8 pinNumber{0};
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
      pushTo(pinConsumers_[idx], read_gpio(port_, pinNumber) ? T{1} : T{0});
    }
  }

  void handleClose() {
    for (u32 i = 0; i < handleCount_; ++i) {
      clearGpio(handles_[i]);
    }
  }

  u16 port_;
  Array<u8> pins_;
  Vectorized<Pss<T>> pinConsumers_[kMaxPins]{};
  PinSlot slots_[kMaxPins]{};
  u32 handles_[kMaxPins]{};
  u32 handleCount_{0};
  u8 connected_{0};
  MemberCallback<GpioIn<T>, &GpioIn<T>::handleClose> closeCb_{this};
};

template <typename T>
void GpioIn<T>::PinSlot::Cbk::operator()() {
  slot->parent->emitPin(slot->pinNumber);
}

}  // namespace push
