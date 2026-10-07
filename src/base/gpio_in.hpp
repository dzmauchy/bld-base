#pragma once

#include <core/hal.hpp>

namespace push::detail {

template <typename I>
core::function<void(I)> makeGpioIn(const u32       blockId,
                                   const u16       port,
                                   core::array<u8> pins) {
  using T = I::Value;
  constexpr u8 maxPins = 8;
  const auto   count = pins.size() < maxPins ? pins.size() : maxPins;
  if (register_gpio_block) {
    const core::array<u8> activePins(core::span<const u8>{pins.data(), count});
    register_gpio_block(blockId, port, activePins);
  }
  auto pinConsumers = core::make_shared<core::array<core::array<core::function<void(T)> *>>>();
  auto callbacks = core::make_shared<core::array<core::function<void()>>>(count);
  for (u32 i = 0; i < count; ++i) {
    const auto pin = pins[i];
    u32        index = 0;
    while (pins[index] != pin)
      ++index;
    (*callbacks)[i] = [pinConsumers, port, pin, index] {
      if (index < pinConsumers->size()) {
        const auto value = read_gpio(port, pin) ? T{1} : T{0};
        for (auto *sink : (*pinConsumers)[index]) {
          if (sink)
            (*sink)(value);
        }
      }
    };
  }
  auto handles = core::make_shared<core::array<u32>>();
  auto close = core::make_shared<core::function<void()>>([handles] {
    for (const auto handle : *handles)
      clear_gpio(handle);
  });
  return [port, pins = core::detail::move(pins), pinConsumers, callbacks, handles, close](I input) {
    if (input.pins.size() > callbacks->size())
      __builtin_trap();
    const auto count = input.pins.size() < maxPins ? input.pins.size() : maxPins;
    *pinConsumers = core::array<core::array<core::function<void(T)> *>>(count);
    for (core::size_t i = 0; i < count; ++i)
      (*pinConsumers)[i] = core::array<core::function<void(T)> *>(input.pins[i]);
    core::array<u32> registered(handles->size() + callbacks->size());
    for (core::size_t i = 0; i < handles->size(); ++i)
      registered[i] = (*handles)[i];
    for (u32 i = 0; i < callbacks->size(); ++i) {
      registered[handles->size() + i] = set_gpio(port, pins[i], &(*callbacks)[i]);
    }
    *handles = core::detail::move(registered);
    on_close(close.get());
  };
}

} // namespace push::detail
