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

namespace push::f_32::transformers {

/**
 * cos
 * @brief Computes the cosine of the input value
 * @image cos.svg
 */
class CosF32 : public UnaryTransformer<DownstreamInput<f32>, CosF32Output> {
 public:
  explicit CosF32(const u32 blockId) : UnaryTransformer(blockId) {}

 protected:
  [[nodiscard]]
  f32 transform(const f32 value) const override {
    return math::cos(value);
  }
};

/**
 * sin
 * @brief Computes the sine of the input value
 * @image sin.svg
 */
class SinF32 : public UnaryTransformer<DownstreamInput<f32>, SinF32Output> {
 public:
  explicit SinF32(const u32 blockId) : UnaryTransformer(blockId) {}

 protected:
  [[nodiscard]]
  f32 transform(const f32 value) const override {
    return math::sin(value);
  }
};

/**
 * Product
 * @brief Computes the product of the input values
 * @image product.svg
 */
class ProductF32 : public Aggregate<AggregateInput<f32>, ProductF32Output> {
 public:
  explicit ProductF32(const u32 blockId,
                      const u32 precision = 10)
      : Aggregate(blockId,
                  precision) {}

 protected:
  [[nodiscard]]
  f32 combine(const f32 acc,
              const f32 value) const override {
    return acc * value;
  }
};

/**
 * Sum
 * @brief Computes the sum of the input values
 * @image sum.svg
 */
class SumF32 : public Aggregate<AggregateInput<f32>, SumF32Output> {
 public:
  explicit SumF32(const u32 blockId,
                  const u32 precision = 10)
      : Aggregate(blockId,
                  precision) {}

 protected:
  [[nodiscard]]
  f32 combine(const f32 acc,
              const f32 value) const override {
    return acc + value;
  }
};

}  // namespace push::f_32::transformers

namespace push::f_32::sinks {

/**
 * Scope
 * @brief Displays the input values in a scope
 * @image scope.svg
 */
using ScopeF32 = Scope<ScopeInput, ScopeF32Output>;

}  // namespace push::f_32::sinks

namespace push::f_32::sources {

/**
 * GPIO Input
 * @brief Reads the input value from a GPIO pin
 * @image push.gpio_in.svg
 */
using GpioInF32 = GpioIn<GpioInput<f32>>;

/**
 * Constant
 * @brief Constant value
 * @image push.const.svg
 */
using ConstF32 = Constant<DownstreamInput<f32>>;

/**
 * cos
 * @brief Cosine generator
 * @image push.cos-gen.svg
 */
class CosGenF32 : public WaveGen<DownstreamInput<f32>> {
 public:
  explicit CosGenF32(const u32 blockId,
                     const u32 precision = 10,
                     const f32 frequency = 1,
                     const f32 amplitude = 1,
                     const f32 phase = 0)
      : WaveGen(blockId,
                precision,
                frequency,
                amplitude,
                phase) {}

 protected:
  [[nodiscard]]
  f32 wave(const f32 angle) const override {
    return math::cos(angle);
  }
};

/**
 * sin
 * @brief Sine generator
 * @image push.sin-gen.svg
 */
class SinGenF32 : public WaveGen<DownstreamInput<f32>> {
 public:
  explicit SinGenF32(const u32 blockId,
                     const u32 precision = 10,
                     const f32 frequency = 1,
                     const f32 amplitude = 1,
                     const f32 phase = 0)
      : WaveGen(blockId,
                precision,
                frequency,
                amplitude,
                phase) {}

 protected:
  [[nodiscard]]
  f32 wave(const f32 angle) const override {
    return math::sin(angle);
  }
};

/**
 * Random
 * @brief Random generator
 * @image push.rand-gen.svg
 */
using RandGenF32 = RandGen<DownstreamInput<f32>>;

/**
 * Pulse
 * @brief Pulse signal generator
 * @image push.pulse-gen.svg
 */
using PulseGenF32 = PulseGen<DownstreamInput<f32>>;

}  // namespace push::f_32::sources
