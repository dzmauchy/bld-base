#pragma once

#include <base/aggregate.hpp>
#include <base/constant.hpp>
#include <base/gpio_in.hpp>
#include <base/pulse_gen.hpp>
#include <base/rand_gen.hpp>
#include <base/scope.hpp>
#include <base/unary_transformer.hpp>
#include <base/wave_gen.hpp>
#include <core/math/trig.hpp>

/**
 * <namespace icon="push-ns.svg" description="Push Dataflows"/>
 */
namespace push {
/**
 * <namespace icon="push.f32-ns.svg" description="Single precision push dataflows"/>
 */
namespace f32 {

using F32 = ::f32;

/**
 * <namespace icon="push.transformers-ns.svg" description="Transformers"/>
 */
namespace transformers {

/**
 * <block icon="cos.svg" title="cos" description="Computes the cosine of the input value">
 *   <input icon="cos.svg" description="Value whose cosine is computed"/>
 *   <output icon="cos.svg" description="Cosine of the input value"/>
 * </block>
 */
class CosF32 : public UnaryTransformer<F32> {
 public:
  explicit CosF32(u32 blockId) : UnaryTransformer<F32>(blockId) {}

  using v = Pss<F32>*;
  using cos = Pss<F32>*;

 protected:
  [[nodiscard]] F32 transform(F32 value) const override { return math::cos(value); }
};

/**
 * <block icon="sin.svg" title="sin" description="Computes the sine of the input value">
 *   <input icon="sin.svg" description="Value whose sine is computed"/>
 *   <output icon="sin.svg" description="Sine of the input value"/>
 * </block>
 */
class SinF32 : public UnaryTransformer<F32> {
 public:
  explicit SinF32(u32 blockId) : UnaryTransformer<F32>(blockId) {}

  using v = Pss<F32>*;
  using sin = Pss<F32>*;

 protected:
  [[nodiscard]] F32 transform(F32 value) const override { return math::sin(value); }
};

/**
 * <block icon="product.svg" title="Product" description="Computes the product of the input values">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <input icon="product.svg" description="Values to multiply"/>
 *   <output icon="product.svg" description="Product of the input values"/>
 * </block>
 */
class ProductF32 : public Aggregate<F32> {
 public:
  explicit ProductF32(u32 blockId, u32 precision = 10) : Aggregate<F32>(blockId, precision) {}

  using v = Pss<F32>*;
  using p = Pss<F32>*;

 protected:
  [[nodiscard]] F32 combine(F32 acc, F32 value) const override { return acc * value; }
};

/**
 * <block icon="sum.svg" title="Sum" description="Computes the sum of the input values">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <input icon="sum.svg" description="Values to add"/>
 *   <output icon="sum.svg" description="Sum of the input values"/>
 * </block>
 */
class SumF32 : public Aggregate<F32> {
 public:
  explicit SumF32(u32 blockId, u32 precision = 10) : Aggregate<F32>(blockId, precision) {}

  using v = Pss<F32>*;
  using s = Pss<F32>*;

 protected:
  [[nodiscard]] F32 combine(F32 acc, F32 value) const override { return acc + value; }
};

}  // namespace transformers

/**
 * <namespace icon="push.sinks-ns.svg" description="Sinks"/>
 */
namespace sinks {

/**
 * <block icon="scope.svg" title="Scope" description="Displays the input values in a scope">
 *   <conf id="period" type="u32">
 *     <control type="slider" default="60" min="10" max="600" unit="s"/>
 *   </conf>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <output icon="scope.svg" description="Scope channel"/>
 * </block>
 */
class ScopeF32 : public Scope<F32> {
 public:
  explicit ScopeF32(u32 blockId, u32 period = 60, u32 precision = 10) : Scope<F32>(blockId, period, precision) {}

  using sink = Pss<F32>*;
};

}  // namespace sinks

/**
 * <namespace icon="push.sources-ns.svg" description="Sources"/>
 */
namespace sources {

/**
 * <block icon="push.gpio_in.svg" title="GPIO Input" description="Reads the input value from a GPIO pin">
 *   <conf id="port" type="u16">
 *     <control type="text_input" format="u16hex" min="0" max="65535"/>
 *   </conf>
 *   <conf id="pins" type="array">
 *     <control type="set_of_pins">
 *       <length>
 *         <bind type="input" id="pin">
 *           <concept>
 *             <length kind="eq"/>
 *           </concept>
 *         </bind>
 *       </length>
 *       <control name="T" type="spinner" default="0" min="0" max="255"/>
 *       <implementation>the control should show a row of spinners, each spinner per pin</implementation>
 *       <implementation>the control should permit adding and removing pins</implementation>
 *       <implementation>the pin numbers should be editable</implementation>
 *       <implementation>the pin numbers should be unique and sorted ascending</implementation>
 *     </control>
 *   </conf>
 *   <input icon="push.gpio_in.svg" description="Reads one configured GPIO pin">
 *     <concept>
 *       <length>
 *         <bind type="conf" id="pins">
 *           <control>
 *             <length kind="eq"/>
 *           </control>
 *         </bind>
 *       </length>
 *     </concept>
 *   </input>
 * </block>
 */
class GpioInF32 : public GpioIn<F32> {
 public:
  explicit GpioInF32(u32 blockId, u16 port = 0, Array<u8> pins = {0}) : GpioIn<F32>(blockId, port, move(pins)) {}

  using pin = Pss<F32>*;
};

/**
 * <block icon="push.const.svg" title="Constant" description="Constant value">
 *   <implementation>the implementation should propagate the constant value across all streams</implementation>
 *   <conf id="v" type="f32">
 *     <control type="text_input" format="f32" default="1">
 *       <implementation>the control should be able to define a constant value</implementation>
 *     </control>
 *   </conf>
 *   <input icon="push.const.svg" description="Streams that receive the constant value"/>
 * </block>
 */
class ConstF32 : public Constant<F32> {
 public:
  explicit ConstF32(u32 blockId, F32 v = 1) : Constant<F32>(blockId, v) {}

  using v = Pss<F32>*;
};

/**
 * <block icon="push.cos-gen.svg" title="cos" description="Cosine generator">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="frequency" type="f32">
 *     <control type="text_input" default="1" min="0.001" max="100" format="f32" unit="Hz"/>
 *   </conf>
 *   <conf id="amplitude" type="f32">
 *     <control type="text_input" default="1" format="f32"/>
 *   </conf>
 *   <conf id="phase" type="f32">
 *     <control type="text_input" default="0" format="f32" unit="Radians"/>
 *   </conf>
 *   <input icon="push.cos-gen.svg" description="Streams that receive the cosine wave"/>
 * </block>
 */
class CosGenF32 : public WaveGen<F32> {
 public:
  explicit CosGenF32(u32 blockId, u32 precision = 10, F32 frequency = 1, F32 amplitude = 1, F32 phase = 0)
      : WaveGen<F32>(blockId, precision, frequency, amplitude, phase) {}

  using v = Pss<F32>*;

 protected:
  [[nodiscard]] F32 wave(F32 angle) const override { return math::cos(angle); }
};

/**
 * <block icon="push.sin-gen.svg" title="sin" description="Sine generator">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="frequency" type="f32">
 *     <control type="text_input" default="1" min="0.001" max="100" format="f32" unit="Hz"/>
 *   </conf>
 *   <conf id="amplitude" type="f32">
 *     <control type="text_input" default="1" format="f32"/>
 *   </conf>
 *   <conf id="phase" type="f32">
 *     <control type="text_input" default="0" format="f32" unit="Radians"/>
 *   </conf>
 *   <input icon="push.sin-gen.svg" description="Streams that receive the sine wave"/>
 * </block>
 */
class SinGenF32 : public WaveGen<F32> {
 public:
  explicit SinGenF32(u32 blockId, u32 precision = 10, F32 frequency = 1, F32 amplitude = 1, F32 phase = 0)
      : WaveGen<F32>(blockId, precision, frequency, amplitude, phase) {}

  using v = Pss<F32>*;

 protected:
  [[nodiscard]] F32 wave(F32 angle) const override { return math::sin(angle); }
};

/**
 * <block icon="push.rand-gen.svg" title="Random" description="Random generator">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="amplitude" type="f32">
 *     <control type="text_input" default="1" format="f32"/>
 *   </conf>
 *   <input icon="push.rand-gen.svg" description="Streams that receive the random value"/>
 * </block>
 */
class RandGenF32 : public RandGen<F32> {
 public:
  explicit RandGenF32(u32 blockId, u32 precision = 10, F32 amplitude = 1) : RandGen<F32>(blockId, precision, amplitude) {}

  using v = Pss<F32>*;
};

/**
 * <block icon="push.pulse-gen.svg" title="Pulse" description="Pulse signal generator">
 *   <conf id="duty_cycle" type="f32">
 *     <control type="slider" default="0.5" min="0.0" max="1.0" step="0.01"/>
 *   </conf>
 *   <conf id="amplitude" type="f32">
 *     <control type="text_input" default="1" format="f32"/>
 *   </conf>
 *   <conf id="frequency" type="f32">
 *     <control type="text_input" default="1" min="0.001" max="100" format="f32" unit="Hz"/>
 *   </conf>
 *   <conf id="phase" type="f32">
 *     <control type="text_input" default="0" format="f32" unit="Radians"/>
 *   </conf>
 *   <input icon="push.pulse-gen.svg" description="Streams that receive the pulse"/>
 * </block>
 */
class PulseGenF32 : public PulseGen<F32> {
 public:
  explicit PulseGenF32(u32 blockId, F32 dutyCycle = 0.5f, F32 amplitude = 1, F32 frequency = 1, F32 phase = 0)
      : PulseGen<F32>(blockId, dutyCycle, amplitude, frequency, phase) {}

  using v = Pss<F32>*;
};

}  // namespace sources

}  // namespace f32
}  // namespace push
