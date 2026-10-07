#pragma once

#include <core/hal.hpp>

namespace push::detail {

template <typename O>
core::function<O()> makeScope(const u32 blockId,
                              const u32 = 60,
                              const u32 = 10) {
  using T = O::Value;
  auto consumers = core::make_shared<core::array<core::function<void(T)>>>();
  auto channelPointers = core::make_shared<core::array<core::function<void(T)> *>>();
  VectorizedOutput<core::function<void(T)>> channels =
      [blockId, consumers,
       channelPointers](const u8 count) -> core::span<core::function<void(T)> *const> {
    *consumers = core::array<core::function<void(T)>>(count);
    *channelPointers = core::array<core::function<void(T)> *>(count);
    for (u8 i = 0; i < count; ++i) {
      (*consumers)[i] = [blockId, i](const T value) {
        if constexpr (core::detail::same<T, f32>)
          send_value_f32(blockId, i, value);
        else
          send_value_f64(blockId, i, value);
      };
      (*channelPointers)[i] = &(*consumers)[i];
    }
    return *channelPointers;
  };
  return [channels] { return O{.channels = channels}; };
}

} // namespace push::detail
