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
 * Double precision push dataflows
 * @brief Double precision push dataflows
 * @image push.f64-ns.svg
 */
namespace f64 {

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
template <typename I = DownstreamInput<F64>, typename O = CosF64Output>
class CosF64 : public UnaryTransformer<I, O> {
 public:
  explicit CosF64(u32 blockId) : UnaryTransformer<I, O>(blockId) {}

 protected:
  [[nodiscard]] F64 transform(F64 value) const override { return math::cos(value); }
};

/**
 * sin
 * @brief Computes the sine of the input value
 * @image sin.svg
 */
template <typename I = DownstreamInput<F64>, typename O = SinF64Output>
class SinF64 : public UnaryTransformer<I, O> {
 public:
  explicit SinF64(u32 blockId) : UnaryTransformer<I, O>(blockId) {}

 protected:
  [[nodiscard]] F64 transform(F64 value) const override { return math::sin(value); }
};

/**
 * Product
 * @brief Computes the product of the input values
 * @image product.svg
 */
template <typename I = AggregateInput<F64>, typename O = ProductF64Output>
class ProductF64 : public Aggregate<I, O> {
 public:
  explicit ProductF64(u32 blockId, u32 precision = 10) : Aggregate<I, O>(blockId, precision) {}

 protected:
  [[nodiscard]] F64 combine(F64 acc, F64 value) const override { return acc * value; }
};

/**
 * Sum
 * @brief Computes the sum of the input values
 * @image sum.svg
 */
template <typename I = AggregateInput<F64>, typename O = SumF64Output>
class SumF64 : public Aggregate<I, O> {
 public:
  explicit SumF64(u32 blockId, u32 precision = 10) : Aggregate<I, O>(blockId, precision) {}

 protected:
  [[nodiscard]] F64 combine(F64 acc, F64 value) const override { return acc + value; }
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
template <typename I = ScopeInput, typename O = ScopeF64Output>
class ScopeF64 : public Scope<I, O> {
 public:
  explicit ScopeF64(u32 blockId, u32 period = 60, u32 precision = 10) : Scope<I, O>(blockId, period, precision) {}
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
template <typename I = GpioInput<F64>, typename O = void>
class GpioInF64 : public GpioIn<I, O> {
 public:
  explicit GpioInF64(u32 blockId, u16 port = 0, Array<u8> pins = {0}) : GpioIn<I, O>(blockId, port, move(pins)) {}
};

/**
 * Constant
 * @brief Constant value
 * @image push.const.svg
 */
template <typename I = DownstreamInput<F64>, typename O = void>
class ConstF64 : public Constant<I, O> {
 public:
  explicit ConstF64(u32 blockId, F64 v = 1) : Constant<I, O>(blockId, v) {}
};

/**
 * cos
 * @brief Cosine generator
 * @image push.cos-gen.svg
 */
template <typename I = DownstreamInput<F64>, typename O = void>
class CosGenF64 : public WaveGen<I, O> {
 public:
  explicit CosGenF64(u32 blockId, u32 precision = 10, F64 frequency = 1, F64 amplitude = 1, F64 phase = 0)
      : WaveGen<I, O>(blockId, precision, frequency, amplitude, phase) {}

 protected:
  [[nodiscard]] F64 wave(F64 angle) const override { return math::cos(angle); }
};

/**
 * sin
 * @brief Sine generator
 * @image push.sin-gen.svg
 */
template <typename I = DownstreamInput<F64>, typename O = void>
class SinGenF64 : public WaveGen<I, O> {
 public:
  explicit SinGenF64(u32 blockId, u32 precision = 10, F64 frequency = 1, F64 amplitude = 1, F64 phase = 0)
      : WaveGen<I, O>(blockId, precision, frequency, amplitude, phase) {}

 protected:
  [[nodiscard]] F64 wave(F64 angle) const override { return math::sin(angle); }
};

/**
 * Random
 * @brief Random generator
 * @image push.rand-gen.svg
 */
template <typename I = DownstreamInput<F64>, typename O = void>
class RandGenF64 : public RandGen<I, O> {
 public:
  explicit RandGenF64(u32 blockId, u32 precision = 10, F64 amplitude = 1) : RandGen<I, O>(blockId, precision, amplitude) {}
};

/**
 * Pulse
 * @brief Pulse signal generator
 * @image push.pulse-gen.svg
 */
template <typename I = DownstreamInput<F64>, typename O = void>
class PulseGenF64 : public PulseGen<I, O> {
 public:
  explicit PulseGenF64(u32 blockId, F64 dutyCycle = 0.5, F64 amplitude = 1, F64 frequency = 1, F64 phase = 0)
      : PulseGen<I, O>(blockId, dutyCycle, amplitude, frequency, phase) {}
};

}  // namespace sources

}  // namespace f64
}  // namespace push
