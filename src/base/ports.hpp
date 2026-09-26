#pragma once

#include <core/array.hpp>
#include <core/types.hpp>

/**
 * Push
 * @brief Push dataflows.
 * @image push-ns.svg
 */
namespace push {

/**
 * DownstreamInput
 * @brief Consumer streams supplied to a push block.
 * @image input.svg
 */
template <typename T>
struct DownstreamInput {
  /**
   * Value
   * @brief The numeric value carried by the consumer streams.
   * @image type.svg
   */
  using Value = T;

  /**
   * Downstream
   * @brief Streams that receive values emitted by the block.
   * @image consumer.svg
   */
  Vectorized<Consumer<T>> downstream{};
};

/**
 * AggregateInput
 * @brief Downstream streams and the number of aggregate channels.
 * @image input.svg
 */
template <typename T>
struct AggregateInput {
  /**
   * Value
   * @brief The numeric value carried by the consumer streams.
   * @image type.svg
   */
  using Value = T;

  /**
   * Downstream
   * @brief Streams that receive the combined value.
   * @image consumer.svg
   */
  Vectorized<Consumer<T>> downstream{};
  /**
   * Channel count
   * @brief Number of independent consumer channels to create.
   * @image input.svg
   */
  u8 channelCount{0};
};

/**
 * ScopeInput
 * @brief Inputs used to create scope channels.
 * @image input.svg
 */
struct ScopeInput {
  /**
   * Channel count
   * @brief Number of independent scope channels to create.
   * @image scope.svg
   */
  u8 channelCount{0};
};

/**
 * GpioInput
 * @brief Consumer streams for the configured GPIO pins.
 * @image input.svg
 */
template <typename T>
struct GpioInput {
  /**
   * Value
   * @brief The numeric value carried by the consumer streams.
   * @image type.svg
   */
  using Value = T;

  /**
   * Pins
   * @brief Streams per configured pin, in constructor pin order; empty entries leave pins disconnected.
   * @image push.gpio_in.svg
   */
  Array<Vectorized<Consumer<T>>> pins{};
};

/**
 * Single precision push dataflows
 * @brief Single precision push dataflows
 * @image push.f32-ns.svg
 */
namespace f32 {

/**
 * F32
 * @brief The scalar type for this namespace.
 * @image type.svg
 */
using F32 = ::f32;

/**
 * Transformers
 * @brief Transformers
 * @image push.transformers-ns.svg
 */
namespace transformers {

/**
 * CosF32Output
 * @brief The output ports returned by CosF32.
 * @image cos.svg
 */
struct CosF32Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F32;

  /**
   * Cosine input consumer
   * @brief Accepts values whose cosine is pushed to the downstream streams.
   * @image cos.svg
   */
  Consumer<F32>* consumer{nullptr};
};

/**
 * SinF32Output
 * @brief The output ports returned by SinF32.
 * @image sin.svg
 */
struct SinF32Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F32;

  /**
   * Sine input consumer
   * @brief Accepts values whose sine is pushed to the downstream streams.
   * @image sin.svg
   */
  Consumer<F32>* consumer{nullptr};
};

/**
 * ProductF32Output
 * @brief The output ports returned by ProductF32.
 * @image product.svg
 */
struct ProductF32Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F32;

  /**
   * Factor channels
   * @brief One vectorized output containing the consumers for the factors to multiply.
   * @image product.svg
   */
  Vectorized<Consumer<F32>> channels{};
};

/**
 * SumF32Output
 * @brief The output ports returned by SumF32.
 * @image sum.svg
 */
struct SumF32Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F32;

  /**
   * Term channels
   * @brief One vectorized output containing the consumers for the terms to add.
   * @image sum.svg
   */
  Vectorized<Consumer<F32>> channels{};
};

}  // namespace transformers

/**
 * Sinks
 * @brief Sinks
 * @image push.sinks-ns.svg
 */
namespace sinks {

/**
 * ScopeF32Output
 * @brief The output ports returned by ScopeF32.
 * @image scope.svg
 */
struct ScopeF32Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F32;

  /**
   * Scope channels
   * @brief One vectorized output containing independently observed scope consumers.
   * @image scope.svg
   */
  Vectorized<Consumer<F32>> channels{};
};

}  // namespace sinks

}  // namespace f32

/**
 * Double precision push dataflows
 * @brief Double precision push dataflows
 * @image push.f64-ns.svg
 */
namespace f64 {

/**
 * F64
 * @brief The scalar type for this namespace.
 * @image type.svg
 */
using F64 = ::f64;

/**
 * Transformers
 * @brief Transformers
 * @image push.transformers-ns.svg
 */
namespace transformers {

/**
 * CosF64Output
 * @brief The output ports returned by CosF64.
 * @image cos.svg
 */
struct CosF64Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F64;

  /**
   * Cosine input consumer
   * @brief Accepts values whose cosine is pushed to the downstream streams.
   * @image cos.svg
   */
  Consumer<F64>* consumer{nullptr};
};

/**
 * SinF64Output
 * @brief The output ports returned by SinF64.
 * @image sin.svg
 */
struct SinF64Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F64;

  /**
   * Sine input consumer
   * @brief Accepts values whose sine is pushed to the downstream streams.
   * @image sin.svg
   */
  Consumer<F64>* consumer{nullptr};
};

/**
 * ProductF64Output
 * @brief The output ports returned by ProductF64.
 * @image product.svg
 */
struct ProductF64Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F64;

  /**
   * Factor channels
   * @brief One vectorized output containing the consumers for the factors to multiply.
   * @image product.svg
   */
  Vectorized<Consumer<F64>> channels{};
};

/**
 * SumF64Output
 * @brief The output ports returned by SumF64.
 * @image sum.svg
 */
struct SumF64Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F64;

  /**
   * Term channels
   * @brief One vectorized output containing the consumers for the terms to add.
   * @image sum.svg
   */
  Vectorized<Consumer<F64>> channels{};
};

}  // namespace transformers

/**
 * Sinks
 * @brief Sinks
 * @image push.sinks-ns.svg
 */
namespace sinks {

/**
 * ScopeF64Output
 * @brief The output ports returned by ScopeF64.
 * @image scope.svg
 */
struct ScopeF64Output {
  /**
   * Value
   * @brief The numeric type carried by the output consumers.
   * @image type.svg
   */
  using Value = F64;

  /**
   * Scope channels
   * @brief One vectorized output containing independently observed scope consumers.
   * @image scope.svg
   */
  Vectorized<Consumer<F64>> channels{};
};

}  // namespace sinks

}  // namespace f64

}  // namespace push
