#pragma once

#include <base/aggregate.hpp>
#include <base/constant.hpp>
#include <base/gpio_in.hpp>
#include <base/ports.hpp>
#include <base/pulse_gen.hpp>
#include <base/rand_gen.hpp>
#include <base/scope.hpp>
#include <base/unary_transformer.hpp>
#include <base/wave_gen.hpp>
#include <core/math/trig.hpp>

/**
 * Push Dataflows
 * @brief Push Dataflows
 * @image push-ns.svg
 */
namespace push {
/**
 * Single precision push dataflows
 * @brief Single precision push dataflows
 * @image push.f32-ns.svg
 */
namespace f32 {

/**
 * Transformers
 * @brief Transformers
 * @image push.transformers-ns.svg
 */
namespace transformers {

/**
 * cos
 * @brief Computes the cosine of the input value
 * @image cos.svg
 */
template <typename I = DownstreamInput<F32>, typename O = CosF32Output>
class CosF32 : public UnaryTransformer<I, O> {
 public:
  explicit CosF32(u32 blockId) : UnaryTransformer<I, O>(blockId) {}

 protected:
  [[nodiscard]] F32 transform(F32 value) const override { return math::cos(value); }
};

/**
 * sin
 * @brief Computes the sine of the input value
 * @image sin.svg
 */
template <typename I = DownstreamInput<F32>, typename O = SinF32Output>
class SinF32 : public UnaryTransformer<I, O> {
 public:
  explicit SinF32(u32 blockId) : UnaryTransformer<I, O>(blockId) {}

 protected:
  [[nodiscard]] F32 transform(F32 value) const override { return math::sin(value); }
};

/**
 * Product
 * @brief Computes the product of the input values
 * @image product.svg
 */
template <typename I = AggregateInput<F32>, typename O = ProductF32Output>
class ProductF32 : public Aggregate<I, O> {
 public:
  explicit ProductF32(u32 blockId, u32 precision = 10) : Aggregate<I, O>(blockId, precision) {}

 protected:
  [[nodiscard]] F32 combine(F32 acc, F32 value) const override { return acc * value; }
};

/**
 * Sum
 * @brief Computes the sum of the input values
 * @image sum.svg
 */
template <typename I = AggregateInput<F32>, typename O = SumF32Output>
class SumF32 : public Aggregate<I, O> {
 public:
  explicit SumF32(u32 blockId, u32 precision = 10) : Aggregate<I, O>(blockId, precision) {}

 protected:
  [[nodiscard]] F32 combine(F32 acc, F32 value) const override { return acc + value; }
};

}  // namespace transformers

/**
 * Sinks
 * @brief Sinks
 * @image push.sinks-ns.svg
 */
namespace sinks {

/**
 * Scope
 * @brief Displays the input values in a scope
 * @image scope.svg
 */
template <typename I = ScopeInput, typename O = ScopeF32Output>
class ScopeF32 : public Scope<I, O> {
 public:
  explicit ScopeF32(u32 blockId, u32 period = 60, u32 precision = 10) : Scope<I, O>(blockId, period, precision) {}
};

}  // namespace sinks

/**
 * Sources
 * @brief Sources
 * @image push.sources-ns.svg
 */
namespace sources {

/**
 * GPIO Input
 * @brief Reads the input value from a GPIO pin
 * @image push.gpio_in.svg
 */
template <typename I = GpioInput<F32>, typename O = void>
class GpioInF32 : public GpioIn<I, O> {
 public:
  explicit GpioInF32(u32 blockId, u16 port = 0, Array<u8> pins = {0}) : GpioIn<I, O>(blockId, port, move(pins)) {}
};

/**
 * Constant
 * @brief Constant value
 * @image push.const.svg
 */
template <typename I = DownstreamInput<F32>, typename O = void>
class ConstF32 : public Constant<I, O> {
 public:
  explicit ConstF32(u32 blockId, F32 v = 1) : Constant<I, O>(blockId, v) {}
};

/**
 * cos
 * @brief Cosine generator
 * @image push.cos-gen.svg
 */
template <typename I = DownstreamInput<F32>, typename O = void>
class CosGenF32 : public WaveGen<I, O> {
 public:
  explicit CosGenF32(u32 blockId, u32 precision = 10, F32 frequency = 1, F32 amplitude = 1, F32 phase = 0)
      : WaveGen<I, O>(blockId, precision, frequency, amplitude, phase) {}

 protected:
  [[nodiscard]] F32 wave(F32 angle) const override { return math::cos(angle); }
};

/**
 * sin
 * @brief Sine generator
 * @image push.sin-gen.svg
 */
template <typename I = DownstreamInput<F32>, typename O = void>
class SinGenF32 : public WaveGen<I, O> {
 public:
  explicit SinGenF32(u32 blockId, u32 precision = 10, F32 frequency = 1, F32 amplitude = 1, F32 phase = 0)
      : WaveGen<I, O>(blockId, precision, frequency, amplitude, phase) {}

 protected:
  [[nodiscard]] F32 wave(F32 angle) const override { return math::sin(angle); }
};

/**
 * Random
 * @brief Random generator
 * @image push.rand-gen.svg
 */
template <typename I = DownstreamInput<F32>, typename O = void>
class RandGenF32 : public RandGen<I, O> {
 public:
  explicit RandGenF32(u32 blockId, u32 precision = 10, F32 amplitude = 1) : RandGen<I, O>(blockId, precision, amplitude) {}
};

/**
 * Pulse
 * @brief Pulse signal generator
 * @image push.pulse-gen.svg
 */
template <typename I = DownstreamInput<F32>, typename O = void>
class PulseGenF32 : public PulseGen<I, O> {
 public:
  explicit PulseGenF32(u32 blockId, F32 dutyCycle = 0.5f, F32 amplitude = 1, F32 frequency = 1, F32 phase = 0)
      : PulseGen<I, O>(blockId, dutyCycle, amplitude, frequency, phase) {}
};

}  // namespace sources

}  // namespace f32
}  // namespace push
