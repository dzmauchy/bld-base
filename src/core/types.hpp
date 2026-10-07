#pragma once

#include <core/lib.hpp>

using Bool = bool;

using i8 = signed char;
using u8 = unsigned char;
using i16 = short;
using u16 = unsigned short;
using i32 = int;
using u32 = unsigned int;
using i64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;

/**
 * VectorizedInput
 * @brief A vectorized input port holding consumer pointers.
 * @details Borrows its pointer list and consumers; both must remain alive at the same addresses
 * while the view is used. Built-in blocks copy the pointer list when wired.
 * @image type.svg
 */
template <typename T> using VectorizedInput = core::span<T *const>;

/**
 * VectorizedOutput
 * @brief A vectorized output port accepting a channel count and returning consumer pointers.
 * @details Owns its callable and returns a borrowed view. Keep the callable and its channel
 * storage alive; rebuilding channels invalidates earlier views and consumer pointers.
 * @image type.svg
 */
template <typename T> using VectorizedOutput = core::function<core::span<T *const>(u8)>;
