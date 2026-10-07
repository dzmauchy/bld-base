#pragma once

#include <core/types.hpp>
#include <functional>
#include <vector>

namespace push {

/**
 * DownstreamInput
 * @brief Consumer streams supplied to a push block.
 * @image input.svg
 */
template <typename T> struct DownstreamInput {
  using Value = T;

  /**
   * Downstream
   * @brief Streams that receive values emitted by the block.
   * @image consumer.svg
   */
  VectorizedInput<std::function<void(T)>> downstream{};
};

/**
 * GpioInput
 * @brief Consumer streams for the configured GPIO pins.
 * @image input.svg
 */
template <typename T> struct GpioInput {
  using Value = T;

  /**
   * Pins
   * @brief Streams per configured pin, in constructor pin order; empty entries leave pins
   * disconnected.
   * @image push.gpio_in.svg
   */
  std::vector<VectorizedInput<std::function<void(T)>>> pins{};
};

} // namespace push

namespace push::f_32::transformers {

/**
 * CosF32Output
 * @brief The output ports returned by CosF32.
 * @image cos.svg
 */
struct CosF32Output {
  using Value = f32;

  /**
   * Cosine input consumer
   * @brief Accepts values whose cosine is pushed to the downstream streams.
   * @image cos.svg
   */
  std::function<void(f32)> *consumer{nullptr};
};

/**
 * SinF32Output
 * @brief The output ports returned by SinF32.
 * @image sin.svg
 */
struct SinF32Output {
  using Value = f32;

  /**
   * Sine input consumer
   * @brief Accepts values whose sine is pushed to the downstream streams.
   * @image sin.svg
   */
  std::function<void(f32)> *consumer{nullptr};
};

/**
 * ProductF32Output
 * @brief The output ports returned by ProductF32.
 * @image product.svg
 */
struct ProductF32Output {
  using Value = f32;

  /**
   * Factor channels
   * @brief One vectorized output containing the consumers for the factors to multiply.
   * @image product.svg
   */
  VectorizedOutput<std::function<void(f32)>> channels{};
};

/**
 * SumF32Output
 * @brief The output ports returned by SumF32.
 * @image sum.svg
 */
struct SumF32Output {
  using Value = f32;

  /**
   * Term channels
   * @brief One vectorized output containing the consumers for the terms to add.
   * @image sum.svg
   */
  VectorizedOutput<std::function<void(f32)>> channels{};
};

} // namespace push::f_32::transformers

namespace push::f_32::sinks {

/**
 * ScopeF32Output
 * @brief The output ports returned by ScopeF32.
 * @image scope.svg
 */
struct ScopeF32Output {
  using Value = f32;

  /**
   * Scope channels
   * @brief One vectorized output containing independently observed scope consumers.
   * @image scope.svg
   */
  VectorizedOutput<std::function<void(f32)>> channels{};
};

} // namespace push::f_32::sinks

namespace push::f_64::transformers {

/**
 * CosF64Output
 * @brief The output ports returned by CosF64.
 * @image cos.svg
 */
struct CosF64Output {
  using Value = f64;

  /**
   * Cosine input consumer
   * @brief Accepts values whose cosine is pushed to the downstream streams.
   * @image cos.svg
   */
  std::function<void(f64)> *consumer{nullptr};
};

/**
 * SinF64Output
 * @brief The output ports returned by SinF64.
 * @image sin.svg
 */
struct SinF64Output {
  using Value = f64;

  /**
   * Sine input consumer
   * @brief Accepts values whose sine is pushed to the downstream streams.
   * @image sin.svg
   */
  std::function<void(f64)> *consumer{nullptr};
};

/**
 * ProductF64Output
 * @brief The output ports returned by ProductF64.
 * @image product.svg
 */
struct ProductF64Output {
  using Value = f64;

  /**
   * Factor channels
   * @brief One vectorized output containing the consumers for the factors to multiply.
   * @image product.svg
   */
  VectorizedOutput<std::function<void(f64)>> channels{};
};

/**
 * SumF64Output
 * @brief The output ports returned by SumF64.
 * @image sum.svg
 */
struct SumF64Output {
  using Value = f64;

  /**
   * Term channels
   * @brief One vectorized output containing the consumers for the terms to add.
   * @image sum.svg
   */
  VectorizedOutput<std::function<void(f64)>> channels{};
};

} // namespace push::f_64::transformers

namespace push::f_64::sinks {

/**
 * ScopeF64Output
 * @brief The output ports returned by ScopeF64.
 * @image scope.svg
 */
struct ScopeF64Output {
  using Value = f64;

  /**
   * Scope channels
   * @brief One vectorized output containing independently observed scope consumers.
   * @image scope.svg
   */
  VectorizedOutput<std::function<void(f64)>> channels{};
};

} // namespace push::f_64::sinks
