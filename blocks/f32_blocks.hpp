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

/*{"kind":"namespace","name":"Push Dataflows","icon":"push-ns.svg","description":"Push Dataflows"}*/
namespace push {
/*{"kind":"namespace","name":"Single precision","icon":"push.f32-ns.svg","description":"Single precision push dataflows"}*/
namespace f32 {

using F32 = ::f32;

/*{"kind":"namespace","name":"Transformers","icon":"push.transformers-ns.svg","description":"Transformers"}*/
namespace transformers {

/*{"kind":"block","id":"cos_f32","ns":["push","f32","transformers"],"icon":"cos.svg","title":"cos","description":"Computes the cosine of the input value"}*/
class CosF32 : public UnaryTransformer<F32> {
 public:
  explicit CosF32(u32 blockId) : UnaryTransformer<F32>(blockId) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
  /*{"kind":"output","vector":false,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using cos = Pss<F32>*;

 protected:
  [[nodiscard]] F32 transform(F32 value) const override { return math::cos(value); }
};

/*{"kind":"block","id":"sin_f32","ns":["push","f32","transformers"],"icon":"sin.svg","title":"sin","description":"Computes the sine of the input value"}*/
class SinF32 : public UnaryTransformer<F32> {
 public:
  explicit SinF32(u32 blockId) : UnaryTransformer<F32>(blockId) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
  /*{"kind":"output","vector":false,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using sin = Pss<F32>*;

 protected:
  [[nodiscard]] F32 transform(F32 value) const override { return math::sin(value); }
};

/*{"kind":"block","id":"product_f32","ns":["push","f32","transformers"],"icon":"product.svg","title":"Product","description":"Computes the product of the input values"}*/
class ProductF32 : public Aggregate<F32> {
 public:
  explicit ProductF32(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10)
      : Aggregate<F32>(blockId, precision) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
  /*{"kind":"output","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using p = Pss<F32>*;

 protected:
  [[nodiscard]] F32 combine(F32 acc, F32 value) const override { return acc * value; }
};

/*{"kind":"block","id":"sum_f32","ns":["push","f32","transformers"],"icon":"sum.svg","title":"Sum","description":"Computes the sum of the input values"}*/
class SumF32 : public Aggregate<F32> {
 public:
  explicit SumF32(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10)
      : Aggregate<F32>(blockId, precision) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
  /*{"kind":"output","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using s = Pss<F32>*;

 protected:
  [[nodiscard]] F32 combine(F32 acc, F32 value) const override { return acc + value; }
};

}  // namespace transformers

/*{"kind":"namespace","name":"Sinks","icon":"push.sinks-ns.svg","description":"Sinks"}*/
namespace sinks {

/*{"kind":"block","id":"scope_f32","ns":["push","f32","sinks"],"icon":"scope.svg","title":"Scope","description":"Displays the input values in a scope"}*/
class ScopeF32 : public Scope<F32> {
 public:
  explicit ScopeF32(
      u32 blockId,
      /*{"kind":"conf","id":"period","type":{"raw":"u32"},"control":{"type":"slider","default":60,"min":10,"max":600,"unit":"s"}}*/
      u32 period = 60,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10)
      : Scope<F32>(blockId, period, precision) {}

  /*{"kind":"output","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using sink = Pss<F32>*;
};

}  // namespace sinks

/*{"kind":"namespace","name":"Sources","icon":"push.sources-ns.svg","description":"Sources"}*/
namespace sources {

/*{"kind":"block","id":"gpio_in_f32","ns":["push","f32","sources"],"icon":"push.gpio_in.svg","title":"GPIO Input","description":"Reads the input value from a GPIO pin"}*/
class GpioInF32 : public GpioIn<F32> {
 public:
  explicit GpioInF32(
      u32 blockId,
      /*{"kind":"conf","id":"port","type":{"raw":"u16"},"control":{"type":"text_input","format":"u16hex","min":0,"max":65535}}*/
      u16 port = 0,
      /*{"kind":"conf","id":"pins","type":{"raw":"array","args":{"T":{"raw":"u8"}}},"control":{"type":"set_of_pins","length":{"bind":{"type":"input","id":"pin","concept":{"length":{"kind":"eq"}}}},"args":{"T":{"type":"spinner","default":0,"min":0,"max":255}},"implementation":["the control should show a row of spinners, each spinner per pin","the control should permit adding and removing pins","the pin numbers should be editable","the pin numbers should be unique and sorted ascending"]}}*/
      Array<u8> pins = {0})
      : GpioIn<F32>(blockId, port, move(pins)) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}},"concept":{"length":{"bind":{"type":"conf","id":"pins","control":{"length":{"kind":"eq"}}}}}}*/
  using pin = Pss<F32>*;
};

/*{"kind":"block","id":"const_f32","ns":["push","f32","sources"],"icon":"push.const.svg","title":"Constant","description":"Constant value","implementation":["the implementation should propagate the constant value across all streams"]}*/
class ConstF32 : public Constant<F32> {
 public:
  explicit ConstF32(
      u32 blockId,
      /*{"kind":"conf","id":"v","type":{"raw":"f32"},"control":{"type":"text_input","format":"f32","default":1,"implementation":["the control should be able to define a constant value"]}}*/
      F32 v = 1)
      : Constant<F32>(blockId, v) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
};

/*{"kind":"block","id":"cos_gen_f32","ns":["push","f32","sources"],"icon":"push.cos-gen.svg","title":"cos","description":"Cosine generator"}*/
class CosGenF32 : public WaveGen<F32> {
 public:
  explicit CosGenF32(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10,
      /*{"kind":"conf","id":"frequency","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"min":0.001,"max":100,"format":"f32","unit":"Hz"}}*/
      F32 frequency = 1,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"format":"f32"}}*/
      F32 amplitude = 1,
      /*{"kind":"conf","id":"phase","type":{"raw":"f32"},"control":{"type":"text_input","default":0,"format":"f32","unit":"Radians"}}*/
      F32 phase = 0)
      : WaveGen<F32>(blockId, precision, frequency, amplitude, phase) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;

 protected:
  [[nodiscard]] F32 wave(F32 angle) const override { return math::cos(angle); }
};

/*{"kind":"block","id":"sin_gen_f32","ns":["push","f32","sources"],"icon":"push.sin-gen.svg","title":"sin","description":"Sine generator"}*/
class SinGenF32 : public WaveGen<F32> {
 public:
  explicit SinGenF32(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10,
      /*{"kind":"conf","id":"frequency","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"min":0.001,"max":100,"format":"f32","unit":"Hz"}}*/
      F32 frequency = 1,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"format":"f32"}}*/
      F32 amplitude = 1,
      /*{"kind":"conf","id":"phase","type":{"raw":"f32"},"control":{"type":"text_input","default":0,"format":"f32","unit":"Radians"}}*/
      F32 phase = 0)
      : WaveGen<F32>(blockId, precision, frequency, amplitude, phase) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;

 protected:
  [[nodiscard]] F32 wave(F32 angle) const override { return math::sin(angle); }
};

/*{"kind":"block","id":"rand_gen_f32","ns":["push","f32","sources"],"icon":"push.rand-gen.svg","title":"Random","description":"Random generator"}*/
class RandGenF32 : public RandGen<F32> {
 public:
  explicit RandGenF32(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"format":"f32"}}*/
      F32 amplitude = 1)
      : RandGen<F32>(blockId, precision, amplitude) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
};

/*{"kind":"block","id":"pulse_gen_f32","ns":["push","f32","sources"],"icon":"push.pulse-gen.svg","title":"Pulse","description":"Pulse signal generator"}*/
class PulseGenF32 : public PulseGen<F32> {
 public:
  explicit PulseGenF32(
      u32 blockId,
      /*{"kind":"conf","id":"duty_cycle","type":{"raw":"f32"},"control":{"type":"slider","default":0.5,"min":0.0,"max":1.0,"step":0.01}}*/
      F32 dutyCycle = 0.5f,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"format":"f32"}}*/
      F32 amplitude = 1,
      /*{"kind":"conf","id":"frequency","type":{"raw":"f32"},"control":{"type":"text_input","default":1,"min":0.001,"max":100,"format":"f32","unit":"Hz"}}*/
      F32 frequency = 1,
      /*{"kind":"conf","id":"phase","type":{"raw":"f32"},"control":{"type":"text_input","default":0,"format":"f32","unit":"Radians"}}*/
      F32 phase = 0)
      : PulseGen<F32>(blockId, dutyCycle, amplitude, frequency, phase) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f32"}}}}*/
  using v = Pss<F32>*;
};

}  // namespace sources

}  // namespace F32
}  // namespace push
