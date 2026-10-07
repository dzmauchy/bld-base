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
#include <cmath>
#include <functional>

namespace push::f_64::transformers {

/**
 * cos
 * @brief Computes the cosine of the input value
 * @image cos.svg
 */
inline std::function<CosF64Output(DownstreamInput<f64>)> CosF64(const u32 blockId) {
  return detail::makeUnaryTransformer<DownstreamInput<f64>, CosF64Output>(
      blockId, [](const f64 value) { return std::cos(value); });
}

/**
 * sin
 * @brief Computes the sine of the input value
 * @image sin.svg
 */
inline std::function<SinF64Output(DownstreamInput<f64>)> SinF64(const u32 blockId) {
  return detail::makeUnaryTransformer<DownstreamInput<f64>, SinF64Output>(
      blockId, [](const f64 value) { return std::sin(value); });
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
inline std::function<ProductF64Output(DownstreamInput<f64>)> ProductF64(const u32 blockId,
                                                                        const u32 precision = 10) {
  return detail::makeAggregate<DownstreamInput<f64>, ProductF64Output>(
      blockId, precision, [](const f64 acc, const f64 value) { return acc * value; });
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
inline std::function<SumF64Output(DownstreamInput<f64>)> SumF64(const u32 blockId,
                                                                const u32 precision = 10) {
  return detail::makeAggregate<DownstreamInput<f64>, SumF64Output>(
      blockId, precision, [](const f64 acc, const f64 value) { return acc + value; });
}

} // namespace push::f_64::transformers

namespace push::f_64::sinks {

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
inline std::function<ScopeF64Output()> ScopeF64(const u32 blockId,
                                                const u32 period = 60,
                                                const u32 precision = 10) {
  return detail::makeScope<ScopeF64Output>(blockId, period, precision);
}

} // namespace push::f_64::sinks

namespace push::f_64::sources {

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
inline std::function<void(GpioInput<f64>)> GpioInF64(const u32       blockId,
                                                     const u16       port = 0,
                                                     std::vector<u8> pins = {0}) {
  return detail::makeGpioIn<GpioInput<f64>>(blockId, port, std::move(pins));
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
inline std::function<void(DownstreamInput<f64>)> ConstF64(const u32 blockId,
                                                          const f64 value = 1) {
  return detail::makeConstant<DownstreamInput<f64>>(blockId, value);
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
inline std::function<void(DownstreamInput<f64>)> CosGenF64(const u32 blockId,
                                                           const u32 precision = 10,
                                                           const f64 frequency = 1,
                                                           const f64 amplitude = 1,
                                                           const f64 phase = 0) {
  return detail::makeWaveGen<DownstreamInput<f64>>(blockId, precision, frequency, amplitude, phase,
                                                   [](const f64 angle) { return std::cos(angle); });
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
inline std::function<void(DownstreamInput<f64>)> SinGenF64(const u32 blockId,
                                                           const u32 precision = 10,
                                                           const f64 frequency = 1,
                                                           const f64 amplitude = 1,
                                                           const f64 phase = 0) {
  return detail::makeWaveGen<DownstreamInput<f64>>(blockId, precision, frequency, amplitude, phase,
                                                   [](const f64 angle) { return std::sin(angle); });
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
inline std::function<void(DownstreamInput<f64>)> RandGenF64(const u32 blockId,
                                                            const u32 precision = 10,
                                                            const f64 amplitude = 1) {
  return detail::makeRandGen<DownstreamInput<f64>>(blockId, precision, amplitude);
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
inline std::function<void(DownstreamInput<f64>)> PulseGenF64(const u32 blockId,
                                                             const f64 dutyCycle = f64{0.5},
                                                             const f64 amplitude = 1,
                                                             const f64 frequency = 1,
                                                             const f64 phase = 0) {
  return detail::makePulseGen<DownstreamInput<f64>>(blockId, dutyCycle, amplitude, frequency, phase);
}

} // namespace push::f_64::sources
