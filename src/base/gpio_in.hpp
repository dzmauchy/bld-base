#pragma once

#include <algorithm>
#include <core/hal.hpp>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace push::detail {

template <typename I>
std::function<void(I)> makeGpioIn(const u32,
                                  const u16       port,
                                  std::vector<u8> pins) {
  using T = I::Value;
  constexpr u8 maxPins = 8;
  const auto   count = std::min(pins.size(), static_cast<std::size_t>(maxPins));
  auto pinConsumers = std::make_shared<std::vector<std::vector<std::function<void(T)> *>>>();
  auto callbacks = std::make_shared<std::vector<std::function<void()>>>();
  callbacks->reserve(count);
  for (u32 i = 0; i < count; ++i) {
    const auto pin = pins[i];
    const auto index = static_cast<u32>(std::find(pins.begin(), pins.end(), pin) - pins.begin());
    callbacks->emplace_back([pinConsumers, port, pin, index] {
      if (index < pinConsumers->size()) {
        const auto value = read_gpio(port, pin) ? T{1} : T{0};
        for (auto *sink : (*pinConsumers)[index]) {
          if (sink)
            (*sink)(value);
        }
      }
    });
  }
  auto handles = std::make_shared<std::vector<u32>>();
  auto close = std::make_shared<std::function<void()>>([handles] {
    for (const auto handle : *handles)
      clear_gpio(handle);
  });
  return [port, pins = std::move(pins), pinConsumers, callbacks, handles, close](I input) {
    if (input.pins.size() > maxPins)
      input.pins.resize(maxPins);
    pinConsumers->clear();
    pinConsumers->reserve(input.pins.size());
    for (const auto group : input.pins)
      pinConsumers->emplace_back(group.begin(), group.end());
    for (u32 i = 0; i < callbacks->size(); ++i) {
      handles->push_back(set_gpio(port, pins[i], &(*callbacks)[i]));
    }
    on_close(close.get());
  };
}

} // namespace push::detail
