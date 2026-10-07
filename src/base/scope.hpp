#pragma once

#include <core/hal.hpp>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

namespace push::detail {

template <typename O>
std::function<O()> makeScope(const u32 blockId,
                             const u32 = 60,
                             const u32 = 10) {
  using T = O::Value;
  auto consumers = std::make_shared<std::vector<std::function<void(T)>>>();
  auto channelPointers = std::make_shared<std::vector<std::function<void(T)> *>>();
  VectorizedOutput<std::function<void(T)>> channels =
      [blockId, consumers,
       channelPointers](const u8 count) -> std::span<std::function<void(T)> *const> {
    consumers->clear();
    consumers->reserve(count);
    channelPointers->clear();
    channelPointers->reserve(count);
    for (u8 i = 0; i < count; ++i) {
      consumers->emplace_back([blockId, i](const T value) {
        if constexpr (std::is_same_v<T, f32>)
          send_value_f32(blockId, i, value);
        else
          send_value_f64(blockId, i, value);
      });
      channelPointers->push_back(&consumers->back());
    }
    return *channelPointers;
  };
  return [channels] { return O{.channels = channels}; };
}

} // namespace push::detail
