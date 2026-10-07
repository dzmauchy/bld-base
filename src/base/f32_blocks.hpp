#pragma once

#include <core/math.hpp>

#include <base/aggregate.hpp>
#include <base/constant.hpp>
#include <base/gpio_in.hpp>
#include <base/ports.hpp>
#include <base/pulse_gen.hpp>
#include <base/rand_gen.hpp>
#include <base/scope.hpp>
#include <base/unary_transformer.hpp>
#include <base/wave_gen.hpp>

namespace push::f_32::transformers {

/**
 * cos
 * @brief Computes the cosine of the input value
 * @image cos.svg
 */
inline core::function<CosF32Output(DownstreamInput<f32>)> CosF32(const u32 blockId) {
  return detail::makeUnaryTransformer<DownstreamInput<f32>, CosF32Output>(
      blockId, [](const f32 value) { return core::cos(value); });
}

/**
 * sin
 * @brief Computes the sine of the input value
 * @image sin.svg
 */
inline core::function<SinF32Output(DownstreamInput<f32>)> SinF32(const u32 blockId) {
  return detail::makeUnaryTransformer<DownstreamInput<f32>, SinF32Output>(
      blockId, [](const f32 value) { return core::sin(value); });
}

/**
 * Product
 * @brief Computes the product of the input values
 * @image product.svg
 * @param precision Precision
 *   Interval in milliseconds for combining and emitting aggregated values.
 *   @icon precision.svg
 *   @control number
 *   @min 1
 *   @max 1000
 *   @step 1
 */
inline core::function<ProductF32Output(DownstreamInput<f32>)> ProductF32(const u32 blockId,
                                                                         const u32 precision = 10) {
  return detail::makeAggregate<DownstreamInput<f32>, ProductF32Output>(
      blockId, precision, [](const f32 acc, const f32 value) { return acc * value; });
}

/**
 * Sum
 * @brief Computes the sum of the input values
 * @image sum.svg
 * @param precision Precision
 *   Interval in milliseconds for combining and emitting aggregated values.
 *   @icon precision.svg
 *   @control number
 *   @min 1
 *   @max 1000
 *   @step 1
 */
inline core::function<SumF32Output(DownstreamInput<f32>)> SumF32(const u32 blockId,
                                                                 const u32 precision = 10) {
  return detail::makeAggregate<DownstreamInput<f32>, SumF32Output>(
      blockId, precision, [](const f32 acc, const f32 value) { return acc + value; });
}

} // namespace push::f_32::transformers

namespace push::f_32::sinks {

/**
 * Scope
 * @brief Displays the input values in a scope
 * @image scope.svg
 * @param period Period
 *   Observation update period in seconds.
 *   @icon period.svg
 *   @control slider
 *   @min 1
 *   @max 3600
 *   @step 1
 * @param precision Precision
 *   Observation precision in milliseconds.
 *   @icon precision.svg
 *   @control number
 *   @min 1
 *   @max 1000
 *   @step 1
 */
inline core::function<ScopeF32Output()> ScopeF32(const u32 blockId,
                                                 const u32 period = 60,
                                                 const u32 precision = 10) {
  return detail::makeScope<ScopeF32Output>(blockId, period, precision);
}

} // namespace push::f_32::sinks

namespace push::f_32::sources {

/**
 * GPIO Input
 * @brief Reads the input value from a GPIO pin
 * @image push.gpio_in.svg
 * @param port Port
 *   Hardware GPIO port identifier to monitor.
 *   @icon port.svg
 *   @control number
 *   @min 0
 *   @max 65535
 *   @step 1
 * @param pins Pins
 *   GPIO pin indices to listen to for events.
 *   @icon push.gpio_in.svg
 *   @control text
 */
inline core::function<void(GpioInput<f32>)>
GpioInF32(const u32       blockId,
          const u16       port = 0,
          core::array<u8> pins = core::array<u8>(1,
                                                 u8{0})) {
  return detail::makeGpioIn<GpioInput<f32>>(blockId, port, core::detail::move(pins));
}

/**
 * Constant
 * @brief Constant value
 * @image push.const.svg
 * @param value Value
 *   Value emitted by the constant source.
 *   @icon push.const.svg
 *   @control number
 */
inline core::function<void(DownstreamInput<f32>)> ConstF32(const u32 blockId,
                                                           const f32 value = 1) {
  return detail::makeConstant<DownstreamInput<f32>>(blockId, value);
}

/**
 * cos
 * @brief Cosine generator
 * @image push.cos-gen.svg
 * @param precision Precision
 *   Sampling interval in milliseconds.
 *   @icon precision.svg
 *   @control number
 *   @min 1
 *   @max 1000
 *   @step 1
 * @param frequency Frequency
 *   Frequency of the generated wave in Hertz.
 *   @icon frequency.svg
 *   @control number
 *   @min 0
 *   @step 0.1
 * @param amplitude Amplitude
 *   Peak amplitude of the generated wave.
 *   @icon amplitude.svg
 *   @control number
 * @param phase Phase
 *   Initial phase offset in radians.
 *   @icon phase.svg
 *   @control number
 */
inline core::function<void(DownstreamInput<f32>)> CosGenF32(const u32 blockId,
                                                            const u32 precision = 10,
                                                            const f32 frequency = 1,
                                                            const f32 amplitude = 1,
                                                            const f32 phase = 0) {
  return detail::makeWaveGen<DownstreamInput<f32>>(
      blockId, precision, frequency, amplitude, phase,
      [](const f32 angle) { return core::cos(angle); });
}

/**
 * sin
 * @brief Sine generator
 * @image push.sin-gen.svg
 * @param precision Precision
 *   Sampling interval in milliseconds.
 *   @icon precision.svg
 *   @control number
 *   @min 1
 *   @max 1000
 *   @step 1
 * @param frequency Frequency
 *   Frequency of the generated wave in Hertz.
 *   @icon frequency.svg
 *   @control number
 *   @min 0
 *   @step 0.1
 * @param amplitude Amplitude
 *   Peak amplitude of the generated wave.
 *   @icon amplitude.svg
 *   @control number
 * @param phase Phase
 *   Initial phase offset in radians.
 *   @icon phase.svg
 *   @control number
 */
inline core::function<void(DownstreamInput<f32>)> SinGenF32(const u32 blockId,
                                                            const u32 precision = 10,
                                                            const f32 frequency = 1,
                                                            const f32 amplitude = 1,
                                                            const f32 phase = 0) {
  return detail::makeWaveGen<DownstreamInput<f32>>(
      blockId, precision, frequency, amplitude, phase,
      [](const f32 angle) { return core::sin(angle); });
}

/**
 * Random
 * @brief Random generator
 * @image push.rand-gen.svg
 * @param precision Precision
 *   Sampling interval in milliseconds.
 *   @icon precision.svg
 *   @control number
 *   @min 1
 *   @max 1000
 *   @step 1
 * @param amplitude Amplitude
 *   Scale factor applied to generated random values.
 *   @icon amplitude.svg
 *   @control number
 */
inline core::function<void(DownstreamInput<f32>)> RandGenF32(const u32 blockId,
                                                             const u32 precision = 10,
                                                             const f32 amplitude = 1) {
  return detail::makeRandGen<DownstreamInput<f32>>(blockId, precision, amplitude);
}

/**
 * Pulse
 * @brief Pulse signal generator
 * @image push.pulse-gen.svg
 * @param dutyCycle Duty cycle
 *   Fraction of the period during which the pulse signal is high.
 *   @icon duty_cycle.svg
 *   @control slider
 *   @min 0
 *   @max 1
 *   @step 0.01
 * @param amplitude Amplitude
 *   Pulse signal amplitude.
 *   @icon amplitude.svg
 *   @control number
 * @param frequency Frequency
 *   Frequency of the pulse signal in Hertz.
 *   @icon frequency.svg
 *   @control number
 *   @min 0
 *   @step 0.1
 * @param phase Phase
 *   Initial phase offset in radians.
 *   @icon phase.svg
 *   @control number
 */
inline core::function<void(DownstreamInput<f32>)> PulseGenF32(const u32 blockId,
                                                              const f32 dutyCycle = f32{0.5},
                                                              const f32 amplitude = 1,
                                                              const f32 frequency = 1,
                                                              const f32 phase = 0) {
  return detail::makePulseGen<DownstreamInput<f32>>(blockId, dutyCycle, amplitude, frequency,
                                                    phase);
}

} // namespace push::f_32::sources
