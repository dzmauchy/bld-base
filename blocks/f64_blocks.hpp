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
 * <namespace name="Push Dataflows" icon="push-ns.svg">
 *   <description>Push Dataflows</description>
 * </namespace>
 */
namespace push {
/**
 * <namespace name="Double precision" icon="push.f64-ns.svg">
 *   <description>Double precision push dataflows</description>
 * </namespace>
 */
namespace f64 {

using F64 = ::f64;

/**
 * <namespace name="Transformers" icon="push.transformers-ns.svg">
 *   <description>Transformers</description>
 * </namespace>
 */
namespace transformers {

/**
 * <block id="cos_f64" icon="cos.svg" title="cos">
 *   <description>Computes the cosine of the input value</description>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 *   <output name="cos" vector="false">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </output>
 * </block>
 */
class CosF64 : public UnaryTransformer<F64> {
 public:
  explicit CosF64(u32 blockId) : UnaryTransformer<F64>(blockId) {}

  using v = Pss<F64>*;
  using cos = Pss<F64>*;

 protected:
  [[nodiscard]] F64 transform(F64 value) const override { return math::cos(value); }
};

/**
 * <block id="sin_f64" icon="sin.svg" title="sin">
 *   <description>Computes the sine of the input value</description>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 *   <output name="sin" vector="false">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </output>
 * </block>
 */
class SinF64 : public UnaryTransformer<F64> {
 public:
  explicit SinF64(u32 blockId) : UnaryTransformer<F64>(blockId) {}

  using v = Pss<F64>*;
  using sin = Pss<F64>*;

 protected:
  [[nodiscard]] F64 transform(F64 value) const override { return math::sin(value); }
};

/**
 * <block id="product_f64" icon="product.svg" title="Product">
 *   <description>Computes the product of the input values</description>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 *   <output name="p" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </output>
 * </block>
 */
class ProductF64 : public Aggregate<F64> {
 public:
  explicit ProductF64(u32 blockId, u32 precision = 10) : Aggregate<F64>(blockId, precision) {}

  using v = Pss<F64>*;
  using p = Pss<F64>*;

 protected:
  [[nodiscard]] F64 combine(F64 acc, F64 value) const override { return acc * value; }
};

/**
 * <block id="sum_f64" icon="sum.svg" title="Sum">
 *   <description>Computes the sum of the input values</description>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 *   <output name="s" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </output>
 * </block>
 */
class SumF64 : public Aggregate<F64> {
 public:
  explicit SumF64(u32 blockId, u32 precision = 10) : Aggregate<F64>(blockId, precision) {}

  using v = Pss<F64>*;
  using s = Pss<F64>*;

 protected:
  [[nodiscard]] F64 combine(F64 acc, F64 value) const override { return acc + value; }
};

}  // namespace transformers

/**
 * <namespace name="Sinks" icon="push.sinks-ns.svg">
 *   <description>Sinks</description>
 * </namespace>
 */
namespace sinks {

/**
 * <block id="scope_f64" icon="scope.svg" title="Scope">
 *   <description>Displays the input values in a scope</description>
 *   <conf id="period" type="u32">
 *     <control type="slider" default="60" min="10" max="600" unit="s"/>
 *   </conf>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <output name="sink" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </output>
 * </block>
 */
class ScopeF64 : public Scope<F64> {
 public:
  explicit ScopeF64(u32 blockId, u32 period = 60, u32 precision = 10) : Scope<F64>(blockId, period, precision) {}

  using sink = Pss<F64>*;
};

}  // namespace sinks

/**
 * <namespace name="Sources" icon="push.sources-ns.svg">
 *   <description>Sources</description>
 * </namespace>
 */
namespace sources {

/**
 * <block id="gpio_in_f64" icon="push.gpio_in.svg" title="GPIO Input">
 *   <description>Reads the input value from a GPIO pin</description>
 *   <conf id="port" type="u16">
 *     <control type="text_input" format="u16hex" min="0" max="65535"/>
 *   </conf>
 *   <conf id="pins" type="array">
 *     <arg name="T" type="u8"/>
 *     <control type="set_of_pins">
 *       <length>
 *         <bind type="input" id="pin">
 *           <concept>
 *             <length kind="eq"/>
 *           </concept>
 *         </bind>
 *       </length>
 *       <arg name="T">
 *         <control type="spinner" default="0" min="0" max="255"/>
 *       </arg>
 *       <implementation>the control should show a row of spinners, each spinner per pin</implementation>
 *       <implementation>the control should permit adding and removing pins</implementation>
 *       <implementation>the pin numbers should be editable</implementation>
 *       <implementation>the pin numbers should be unique and sorted ascending</implementation>
 *     </control>
 *   </conf>
 *   <input name="pin" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
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
class GpioInF64 : public GpioIn<F64> {
 public:
  explicit GpioInF64(u32 blockId, u16 port = 0, Array<u8> pins = {0}) : GpioIn<F64>(blockId, port, move(pins)) {}

  using pin = Pss<F64>*;
};

/**
 * <block id="const_f64" icon="push.const.svg" title="Constant">
 *   <description>Constant value</description>
 *   <implementation>the implementation should propagate the constant value across all streams</implementation>
 *   <conf id="v" type="f64">
 *     <control type="text_input" format="f64" default="1">
 *       <implementation>the control should be able to define a constant value</implementation>
 *     </control>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 * </block>
 */
class ConstF64 : public Constant<F64> {
 public:
  explicit ConstF64(u32 blockId, F64 v = 1) : Constant<F64>(blockId, v) {}

  using v = Pss<F64>*;
};

/**
 * <block id="cos_gen_f64" icon="push.cos-gen.svg" title="cos">
 *   <description>Cosine generator</description>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="frequency" type="f64">
 *     <control type="text_input" default="1" min="0.001" max="100" format="f64" unit="Hz"/>
 *   </conf>
 *   <conf id="amplitude" type="f64">
 *     <control type="text_input" default="1" format="f64"/>
 *   </conf>
 *   <conf id="phase" type="f64">
 *     <control type="text_input" default="0" format="f64" unit="Radians"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 * </block>
 */
class CosGenF64 : public WaveGen<F64> {
 public:
  explicit CosGenF64(u32 blockId, u32 precision = 10, F64 frequency = 1, F64 amplitude = 1, F64 phase = 0)
      : WaveGen<F64>(blockId, precision, frequency, amplitude, phase) {}

  using v = Pss<F64>*;

 protected:
  [[nodiscard]] F64 wave(F64 angle) const override { return math::cos(angle); }
};

/**
 * <block id="sin_gen_f64" icon="push.sin-gen.svg" title="sin">
 *   <description>Sine generator</description>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="frequency" type="f64">
 *     <control type="text_input" default="1" min="0.001" max="100" format="f64" unit="Hz"/>
 *   </conf>
 *   <conf id="amplitude" type="f64">
 *     <control type="text_input" default="1" format="f64"/>
 *   </conf>
 *   <conf id="phase" type="f64">
 *     <control type="text_input" default="0" format="f64" unit="Radians"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 * </block>
 */
class SinGenF64 : public WaveGen<F64> {
 public:
  explicit SinGenF64(u32 blockId, u32 precision = 10, F64 frequency = 1, F64 amplitude = 1, F64 phase = 0)
      : WaveGen<F64>(blockId, precision, frequency, amplitude, phase) {}

  using v = Pss<F64>*;

 protected:
  [[nodiscard]] F64 wave(F64 angle) const override { return math::sin(angle); }
};

/**
 * <block id="rand_gen_f64" icon="push.rand-gen.svg" title="Random">
 *   <description>Random generator</description>
 *   <conf id="precision" type="u32">
 *     <control type="slider" default="10" min="1" max="1000" unit="ms"/>
 *   </conf>
 *   <conf id="amplitude" type="f64">
 *     <control type="text_input" default="1" format="f64"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 * </block>
 */
class RandGenF64 : public RandGen<F64> {
 public:
  explicit RandGenF64(u32 blockId, u32 precision = 10, F64 amplitude = 1) : RandGen<F64>(blockId, precision, amplitude) {}

  using v = Pss<F64>*;
};

/**
 * <block id="pulse_gen_f64" icon="push.pulse-gen.svg" title="Pulse">
 *   <description>Pulse signal generator</description>
 *   <conf id="duty_cycle" type="f64">
 *     <control type="slider" default="0.5" min="0.0" max="1.0" step="0.01"/>
 *   </conf>
 *   <conf id="amplitude" type="f64">
 *     <control type="text_input" default="1" format="f64"/>
 *   </conf>
 *   <conf id="frequency" type="f64">
 *     <control type="text_input" default="1" min="0.001" max="100" format="f64" unit="Hz"/>
 *   </conf>
 *   <conf id="phase" type="f64">
 *     <control type="text_input" default="0" format="f64" unit="Radians"/>
 *   </conf>
 *   <input name="v" vector="true">
 *     <type name="pss">
 *       <arg name="T" type="f64"/>
 *     </type>
 *   </input>
 * </block>
 */
class PulseGenF64 : public PulseGen<F64> {
 public:
  explicit PulseGenF64(u32 blockId, F64 dutyCycle = 0.5, F64 amplitude = 1, F64 frequency = 1, F64 phase = 0)
      : PulseGen<F64>(blockId, dutyCycle, amplitude, frequency, phase) {}

  using v = Pss<F64>*;
};

}  // namespace sources

}  // namespace f64
}  // namespace push
