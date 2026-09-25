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
/*{"kind":"namespace","name":"Double precision","icon":"push.f64-ns.svg","description":"Double precision push dataflows"}*/
namespace f64 {

using F64 = ::f64;

/*{"kind":"namespace","name":"Transformers","icon":"push.transformers-ns.svg","description":"Transformers"}*/
namespace transformers {

/*{"kind":"block","id":"cos_f64","ns":["push","f64","transformers"],"icon":"cos.svg","title":"cos","description":"Computes the cosine of the input value"}*/
class CosF64 : public UnaryTransformer<F64> {
 public:
  explicit CosF64(u32 blockId) : UnaryTransformer<F64>(blockId) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
  /*{"kind":"output","vector":false,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using cos = Pss<F64>*;

 protected:
  [[nodiscard]] F64 transform(F64 value) const override { return math::cos(value); }
};

/*{"kind":"block","id":"sin_f64","ns":["push","f64","transformers"],"icon":"sin.svg","title":"sin","description":"Computes the sine of the input value"}*/
class SinF64 : public UnaryTransformer<F64> {
 public:
  explicit SinF64(u32 blockId) : UnaryTransformer<F64>(blockId) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
  /*{"kind":"output","vector":false,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using sin = Pss<F64>*;

 protected:
  [[nodiscard]] F64 transform(F64 value) const override { return math::sin(value); }
};

/*{"kind":"block","id":"product_f64","ns":["push","f64","transformers"],"icon":"product.svg","title":"Product","description":"Computes the product of the input values"}*/
class ProductF64 : public Aggregate<F64> {
 public:
  explicit ProductF64(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10)
      : Aggregate<F64>(blockId, precision) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
  /*{"kind":"output","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using p = Pss<F64>*;

 protected:
  [[nodiscard]] F64 combine(F64 acc, F64 value) const override { return acc * value; }
};

/*{"kind":"block","id":"sum_f64","ns":["push","f64","transformers"],"icon":"sum.svg","title":"Sum","description":"Computes the sum of the input values"}*/
class SumF64 : public Aggregate<F64> {
 public:
  explicit SumF64(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10)
      : Aggregate<F64>(blockId, precision) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
  /*{"kind":"output","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using s = Pss<F64>*;

 protected:
  [[nodiscard]] F64 combine(F64 acc, F64 value) const override { return acc + value; }
};

}  // namespace transformers

/*{"kind":"namespace","name":"Sinks","icon":"push.sinks-ns.svg","description":"Sinks"}*/
namespace sinks {

/*{"kind":"block","id":"scope_f64","ns":["push","f64","sinks"],"icon":"scope.svg","title":"Scope","description":"Displays the input values in a scope"}*/
class ScopeF64 : public Scope<F64> {
 public:
  explicit ScopeF64(
      u32 blockId,
      /*{"kind":"conf","id":"period","type":{"raw":"u32"},"control":{"type":"slider","default":60,"min":10,"max":600,"unit":"s"}}*/
      u32 period = 60,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10)
      : Scope<F64>(blockId, period, precision) {}

  /*{"kind":"output","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using sink = Pss<F64>*;
};

}  // namespace sinks

/*{"kind":"namespace","name":"Sources","icon":"push.sources-ns.svg","description":"Sources"}*/
namespace sources {

/*{"kind":"block","id":"gpio_in_f64","ns":["push","f64","sources"],"icon":"push.gpio_in.svg","title":"GPIO Input","description":"Reads the input value from a GPIO pin"}*/
class GpioInF64 : public GpioIn<F64> {
 public:
  explicit GpioInF64(
      u32 blockId,
      /*{"kind":"conf","id":"port","type":{"raw":"u16"},"control":{"type":"text_input","format":"u16hex","min":0,"max":65535}}*/
      u16 port = 0,
      /*{"kind":"conf","id":"pins","type":{"raw":"array","args":{"T":{"raw":"u8"}}},"control":{"type":"set_of_pins","length":{"bind":{"type":"input","id":"pin","concept":{"length":{"kind":"eq"}}}},"args":{"T":{"type":"spinner","default":0,"min":0,"max":255}},"implementation":["the control should show a row of spinners, each spinner per pin","the control should permit adding and removing pins","the pin numbers should be editable","the pin numbers should be unique and sorted ascending"]}}*/
      Array<u8> pins = {0})
      : GpioIn<F64>(blockId, port, move(pins)) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}},"concept":{"length":{"bind":{"type":"conf","id":"pins","control":{"length":{"kind":"eq"}}}}}}*/
  using pin = Pss<F64>*;
};

/*{"kind":"block","id":"const_f64","ns":["push","f64","sources"],"icon":"push.const.svg","title":"Constant","description":"Constant value","implementation":["the implementation should propagate the constant value across all streams"]}*/
class ConstF64 : public Constant<F64> {
 public:
  explicit ConstF64(
      u32 blockId,
      /*{"kind":"conf","id":"v","type":{"raw":"f64"},"control":{"type":"text_input","format":"f64","default":1,"implementation":["the control should be able to define a constant value"]}}*/
      F64 v = 1)
      : Constant<F64>(blockId, v) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
};

/*{"kind":"block","id":"cos_gen_f64","ns":["push","f64","sources"],"icon":"push.cos-gen.svg","title":"cos","description":"Cosine generator"}*/
class CosGenF64 : public WaveGen<F64> {
 public:
  explicit CosGenF64(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10,
      /*{"kind":"conf","id":"frequency","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"min":0.001,"max":100,"format":"f64","unit":"Hz"}}*/
      F64 frequency = 1,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"format":"f64"}}*/
      F64 amplitude = 1,
      /*{"kind":"conf","id":"phase","type":{"raw":"f64"},"control":{"type":"text_input","default":0,"format":"f64","unit":"Radians"}}*/
      F64 phase = 0)
      : WaveGen<F64>(blockId, precision, frequency, amplitude, phase) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;

 protected:
  [[nodiscard]] F64 wave(F64 angle) const override { return math::cos(angle); }
};

/*{"kind":"block","id":"sin_gen_f64","ns":["push","f64","sources"],"icon":"push.sin-gen.svg","title":"sin","description":"Sine generator"}*/
class SinGenF64 : public WaveGen<F64> {
 public:
  explicit SinGenF64(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10,
      /*{"kind":"conf","id":"frequency","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"min":0.001,"max":100,"format":"f64","unit":"Hz"}}*/
      F64 frequency = 1,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"format":"f64"}}*/
      F64 amplitude = 1,
      /*{"kind":"conf","id":"phase","type":{"raw":"f64"},"control":{"type":"text_input","default":0,"format":"f64","unit":"Radians"}}*/
      F64 phase = 0)
      : WaveGen<F64>(blockId, precision, frequency, amplitude, phase) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;

 protected:
  [[nodiscard]] F64 wave(F64 angle) const override { return math::sin(angle); }
};

/*{"kind":"block","id":"rand_gen_f64","ns":["push","f64","sources"],"icon":"push.rand-gen.svg","title":"Random","description":"Random generator"}*/
class RandGenF64 : public RandGen<F64> {
 public:
  explicit RandGenF64(
      u32 blockId,
      /*{"kind":"conf","id":"precision","type":{"raw":"u32"},"control":{"type":"slider","default":10,"min":1,"max":1000,"unit":"ms"}}*/
      u32 precision = 10,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"format":"f64"}}*/
      F64 amplitude = 1)
      : RandGen<F64>(blockId, precision, amplitude) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
};

/*{"kind":"block","id":"pulse_gen_f64","ns":["push","f64","sources"],"icon":"push.pulse-gen.svg","title":"Pulse","description":"Pulse signal generator"}*/
class PulseGenF64 : public PulseGen<F64> {
 public:
  explicit PulseGenF64(
      u32 blockId,
      /*{"kind":"conf","id":"duty_cycle","type":{"raw":"f64"},"control":{"type":"slider","default":0.5,"min":0.0,"max":1.0,"step":0.01}}*/
      F64 dutyCycle = 0.5,
      /*{"kind":"conf","id":"amplitude","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"format":"f64"}}*/
      F64 amplitude = 1,
      /*{"kind":"conf","id":"frequency","type":{"raw":"f64"},"control":{"type":"text_input","default":1,"min":0.001,"max":100,"format":"f64","unit":"Hz"}}*/
      F64 frequency = 1,
      /*{"kind":"conf","id":"phase","type":{"raw":"f64"},"control":{"type":"text_input","default":0,"format":"f64","unit":"Radians"}}*/
      F64 phase = 0)
      : PulseGen<F64>(blockId, dutyCycle, amplitude, frequency, phase) {}

  /*{"kind":"input","vector":true,"type":{"raw":"pss","args":{"T":{"raw":"f64"}}}}*/
  using v = Pss<F64>*;
};

}  // namespace sources

}  // namespace F64
}  // namespace push
