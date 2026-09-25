#pragma once

#include "blocks/aggregate.hpp"
#include "blocks/constant.hpp"
#include "blocks/gpio_in.hpp"
#include "blocks/pulse_gen.hpp"
#include "blocks/rand_gen.hpp"
#include "blocks/scope.hpp"
#include "blocks/unary_transformer.hpp"
#include "blocks/wave_gen.hpp"
#include "math/trig.hpp"

/**
 * <namespace name="Push Dataflows" icon="push-ns.svg" description="Push Dataflows"/>
 */
namespace push {
/**
 * <namespace name="Single precision" icon="push.f32-ns.svg" description="Single precision push dataflows"/>
 */
namespace f32 {

using F32 = ::f32;

/**
 * <namespace name="Transformers" icon="push.transformers-ns.svg" description="Transformers"/>
 */
namespace transformers {

/**
 * <block id="cos_f32" icon="cos.svg" title="cos" description="Computes the cosine of the input value">
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
 *   <output name="cos" vector="false">
 *     <type name="pss"/>
 *   </output>
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
 * <block id="sin_f32" icon="sin.svg" title="sin" description="Computes the sine of the input value">
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
 *   <output name="sin" vector="false">
 *     <type name="pss"/>
 *   </output>
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
 * <block id="product_f32" icon="product.svg" title="Product" description="Computes the product of the input values">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
 *   <output name="p" vector="true">
 *     <type name="pss"/>
 *   </output>
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
 * <block id="sum_f32" icon="sum.svg" title="Sum" description="Computes the sum of the input values">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
 *   <output name="s" vector="true">
 *     <type name="pss"/>
 *   </output>
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
 * <namespace name="Sinks" icon="push.sinks-ns.svg" description="Sinks"/>
 */
namespace sinks {

/**
 * <block id="scope_f32" icon="scope.svg" title="Scope" description="Displays the input values in a scope">
 *   <conf id="period" type="u32">
 *     <control type="slider" default="60" min="10" max="600" unit="s"/>
 *   </conf>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <output name="sink" vector="true">
 *     <type name="pss"/>
 *   </output>
 * </block>
 */
class ScopeF32 : public Scope<F32> {
 public:
  explicit ScopeF32(u32 blockId, u32 period = 60, u32 precision = 10) : Scope<F32>(blockId, period, precision) {}

  using sink = Pss<F32>*;
};

}  // namespace sinks

/**
 * <namespace name="Sources" icon="push.sources-ns.svg" description="Sources"/>
 */
namespace sources {

/**
 * <block id="gpio_in_f32" icon="push.gpio_in.svg" title="GPIO Input" description="Reads the input value from a GPIO pin">
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
 *   <input name="pin" vector="true">
 *     <type name="pss"/>
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
 * <block id="const_f32" icon="push.const.svg" title="Constant" description="Constant value">
 *   <implementation>the implementation should propagate the constant value across all streams</implementation>
 *   <conf id="v" type="f32">
 *     <control type="text_input" format="f32" default="1">
 *       <implementation>the control should be able to define a constant value</implementation>
 *     </control>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
 * </block>
 */
class ConstF32 : public Constant<F32> {
 public:
  explicit ConstF32(u32 blockId, F32 v = 1) : Constant<F32>(blockId, v) {}

  using v = Pss<F32>*;
};

/**
 * <block id="cos_gen_f32" icon="push.cos-gen.svg" title="cos" description="Cosine generator">
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
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
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
 * <block id="sin_gen_f32" icon="push.sin-gen.svg" title="sin" description="Sine generator">
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
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
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
 * <block id="rand_gen_f32" icon="push.rand-gen.svg" title="Random" description="Random generator">
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="amplitude" type="f32">
 *     <control type="text_input" default="1" format="f32"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
 * </block>
 */
class RandGenF32 : public RandGen<F32> {
 public:
  explicit RandGenF32(u32 blockId, u32 precision = 10, F32 amplitude = 1) : RandGen<F32>(blockId, precision, amplitude) {}

  using v = Pss<F32>*;
};

/**
 * <block id="pulse_gen_f32" icon="push.pulse-gen.svg" title="Pulse" description="Pulse signal generator">
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
 *   <input name="v" vector="true">
 *     <type name="pss"/>
 *   </input>
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
