#pragma once

#include <core/types.hpp>
#include <functional>
#include <vector>

/**
 * VectorizedInput
 * @brief A vectorized input port holding consumer pointers.
 * @details Owns its pointer list and borrows the consumers, which must remain alive at the same address while in use.
 * @image type.svg
 */
template <typename T> using VectorizedInput = std::vector<T *>;

/**
 * VectorizedOutput
 * @brief A vectorized output port function accepting channel count and returning consumer pointers.
 * @details Owns its callable; any objects captured by pointer or reference must remain alive while in use.
 * @image type.svg
 */
template <typename T> using VectorizedOutput = std::function<std::vector<T *>(u8)>;
