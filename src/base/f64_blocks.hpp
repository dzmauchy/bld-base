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

namespace push::f_64::transformers {

/**
 * cos
 * @brief Computes the cosine of the input value
 * @image cos.svg
 */
class CosF64 : public UnaryTransformer<DownstreamInput<f64>, CosF64Output> {
public:
  using UnaryTransformer::UnaryTransformer;

protected:
  [[nodiscard]]
  f64 transform(const f64 value) const override {
    return math::cos(value);
  }
};

/**
 * sin
 * @brief Computes the sine of the input value
 * @image sin.svg
 */
class SinF64 : public UnaryTransformer<DownstreamInput<f64>, SinF64Output> {
public:
  using UnaryTransformer::UnaryTransformer;

protected:
  [[nodiscard]]
  f64 transform(const f64 value) const override {
    return math::sin(value);
  }
};

/**
 * Product
 * @brief Computes the product of the input values
 * @image product.svg
 */
class ProductF64 : public Aggregate<DownstreamInput<f64>, ProductF64Output> {
public:
  using Aggregate::Aggregate;

protected:
  [[nodiscard]]
  f64 combine(const f64 acc,
              const f64 value) const override {
    return acc * value;
  }
};

/**
 * Sum
 * @brief Computes the sum of the input values
 * @image sum.svg
 */
class SumF64 : public Aggregate<DownstreamInput<f64>, SumF64Output> {
public:
  using Aggregate::Aggregate;

protected:
  [[nodiscard]]
  f64 combine(const f64 acc,
              const f64 value) const override {
    return acc + value;
  }
};

} // namespace push::f_64::transformers

namespace push::f_64::sinks {

/**
 * Scope
 * @brief Displays the input values in a scope
 * @image scope.svg
 */
using ScopeF64 = Scope<ScopeF64Output>;

} // namespace push::f_64::sinks

namespace push::f_64::sources {

/**
 * GPIO Input
 * @brief Reads the input value from a GPIO pin
 * @image push.gpio_in.svg
 */
using GpioInF64 = GpioIn<GpioInput<f64>>;

/**
 * Constant
 * @brief Constant value
 * @image push.const.svg
 */
using ConstF64 = Constant<DownstreamInput<f64>>;

/**
 * cos
 * @brief Cosine generator
 * @image push.cos-gen.svg
 */
class CosGenF64 : public WaveGen<DownstreamInput<f64>> {
public:
  using WaveGen::WaveGen;

protected:
  [[nodiscard]]
  f64 wave(const f64 angle) const override {
    return math::cos(angle);
  }
};

/**
 * sin
 * @brief Sine generator
 * @image push.sin-gen.svg
 */
class SinGenF64 : public WaveGen<DownstreamInput<f64>> {
public:
  using WaveGen::WaveGen;

protected:
  [[nodiscard]]
  f64 wave(const f64 angle) const override {
    return math::sin(angle);
  }
};

/**
 * Random
 * @brief Random generator
 * @image push.rand-gen.svg
 */
using RandGenF64 = RandGen<DownstreamInput<f64>>;

/**
 * Pulse
 * @brief Pulse signal generator
 * @image push.pulse-gen.svg
 */
using PulseGenF64 = PulseGen<DownstreamInput<f64>>;

} // namespace push::f_64::sources
