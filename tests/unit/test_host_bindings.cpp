#include <array>
#include <cmath>
#include <doctest/doctest.h>
#include <limits>

#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <core/hal.hpp>
#include <numbers>
#include <type_traits>

#include "../mock_runtime.hpp"

TEST_CASE_TEMPLATE("Host C bindings drive time, callbacks, GPIO and observations",
                   T,
                   f32,
                   f64) {
  constexpr auto Scope = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32;
    else
      return push::f_64::sinks::ScopeF64;
  }();
  constexpr auto Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32;
    else
      return push::f_64::sources::SinGenF64;
  }();
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();

  MockRuntime::reset();
  auto scope = Scope(0, 60, 10);
  auto sine = Sine(1, 25, T{1}, T{1}, T{0});
  auto gpio = Gpio(2, 7, core::array<u8>(1, u8{3}));
  auto output = scope();
  auto channels = output.channels(2);
  sine({.downstream = std::array{channels[0]}});
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{channels[1]}}}});

  MockRuntime::setNow(1000);
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  CHECK_EQ(MockRuntime::activeGpioCount(), 1);

  MockRuntime::setNow(1250);
  MockRuntime::tick();
  MockRuntime::emitGpio(7, 3, true);
  CHECK_EQ(get_time(), 1250);
  CHECK(read_gpio(7, 3));
  if constexpr (std::is_same_v<T, f32>) {
    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(1));
    CHECK_EQ(MockRuntime::lastF32(0, 1), 1);
  } else {
    CHECK(MockRuntime::lastF64(0, 0) == doctest::Approx(1));
    CHECK_EQ(MockRuntime::lastF64(0, 1), 1);
  }

  MockRuntime::setRandom(0.25f);
  CHECK_EQ(random_of<T>(), T{0.25});

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("Wave and pulse generators wrap negative phases and repeated periods",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       sineValue = 0;
  T                       pulseValue = 0;
  core::function<void(T)> receiveSine = [&sineValue](const T value) { sineValue = value; };
  core::function<void(T)> receivePulse = [&pulseValue](const T value) { pulseValue = value; };
  constexpr auto          phase = -std::numbers::pi_v<T> / T{2};
  auto                    sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32(0, 10, T{1}, T{1}, phase);
    else
      return push::f_64::sources::SinGenF64(0, 10, T{1}, T{1}, phase);
  }();
  auto pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(1, T{0.5}, T{1}, T{1}, phase);
    else
      return push::f_64::sources::PulseGenF64(1, T{0.5}, T{1}, T{1}, phase);
  }();
  sine({.downstream = std::array{&receiveSine}});
  pulse({.downstream = std::array{&receivePulse}});
  MockRuntime::start();
  for (const u64 time : {u64{0}, u64{500}, u64{1500}}) {
    MockRuntime::setNow(time);
    MockRuntime::tick();
    CHECK(sineValue == doctest::Approx(time == 0 ? -1 : 1));
    CHECK_EQ(pulseValue, time == 0 ? T{0} : T{1});
  }
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Zero-frequency pulse treats a half-turn phase as the low side of the duty edge",
                   T,
                   f32,
                   f64) {
  constexpr auto Pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32;
    else
      return push::f_64::sources::PulseGenF64;
  }();
  const T pi = std::numbers::pi_v<T>;
  for (const T phase : {pi, -pi}) {
    CAPTURE(phase);
    MockRuntime::reset();
    T                       received = T{1};
    u32                     calls = 0;
    core::function<void(T)> receive = [&](const T value) {
      received = value;
      ++calls;
    };
    auto pulse = Pulse(0, T{0.5}, T{6}, T{0}, phase);
    pulse({.downstream = std::array{&receive}});
    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);
    for (const u64 time : {u64{0}, u64{500}, u64{5000}}) {
      MockRuntime::setNow(time);
      MockRuntime::tick();
      CHECK_EQ(received, T{0});
    }
    CHECK_EQ(calls, 3);
    MockRuntime::close();
    MockRuntime::tick();
    CHECK_EQ(calls, 3);
  }
}

TEST_CASE_TEMPLATE("Unary sine and cosine keep odd and even results past one full turn",
                   T,
                   f32,
                   f64) {
  constexpr auto Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SinF32;
    else
      return push::f_64::transformers::SinF64;
  }();
  constexpr auto Cosine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::CosF32;
    else
      return push::f_64::transformers::CosF64;
  }();
  const T pi = std::numbers::pi_v<T>;
  for (const T turns : {T{1.5}, T{2.5}}) {
    CAPTURE(turns);
    MockRuntime::reset();
    T                       sineValue = T{2};
    T                       cosineValue = T{2};
    core::function<void(T)> receiveSine = [&](const T value) { sineValue = value; };
    core::function<void(T)> receiveCosine = [&](const T value) { cosineValue = value; };
    auto sine = Sine(0);
    auto cosine = Cosine(1);
    auto sineConsumer = sine({.downstream = std::array{&receiveSine}}).consumer;
    auto cosineConsumer = cosine({.downstream = std::array{&receiveCosine}}).consumer;
    const T angle = pi * turns;
    (*sineConsumer)(angle);
    (*cosineConsumer)(angle);
    CHECK_EQ(sineValue, turns == T{1.5} ? T{-1} : T{1});
    CHECK(cosineValue == doctest::Approx(0));
    (*sineConsumer)(-angle);
    (*cosineConsumer)(-angle);
    CHECK_EQ(sineValue, turns == T{1.5} ? T{1} : T{-1});
    CHECK(cosineValue == doctest::Approx(0));
  }
}

TEST_CASE_TEMPLATE("Scope keeps non-finite samples on the channel that received them",
                   T,
                   f32,
                   f64) {
  constexpr auto Scope = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32;
    else
      return push::f_64::sinks::ScopeF64;
  }();
  MockRuntime::reset();
  auto       scope = Scope(4, 60, 10);
  auto       output = scope();
  auto       channels = output.channels(2);
  const auto nan = std::numeric_limits<T>::quiet_NaN();
  const auto positive = std::numeric_limits<T>::infinity();
  const auto negative = -positive;
  (*channels[0])(nan);
  (*channels[1])(positive);
  if constexpr (std::is_same_v<T, f32>) {
    CHECK(MockRuntime::hasF32(4, 0));
    CHECK(std::isnan(MockRuntime::lastF32(4, 0)));
    CHECK_EQ(MockRuntime::lastF32(4, 1), positive);
    (*channels[0])(negative);
    CHECK_EQ(MockRuntime::lastF32(4, 0), negative);
    CHECK_EQ(MockRuntime::lastF32(4, 1), positive);
  } else {
    CHECK(MockRuntime::hasF64(4, 0));
    CHECK(std::isnan(MockRuntime::lastF64(4, 0)));
    CHECK_EQ(MockRuntime::lastF64(4, 1), positive);
    (*channels[0])(negative);
    CHECK_EQ(MockRuntime::lastF64(4, 0), negative);
    CHECK_EQ(MockRuntime::lastF64(4, 1), positive);
  }
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("Product preserves subnormal factors and publishes underflow as zero",
                   T,
                   f32,
                   f64) {
  constexpr auto Product = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::ProductF32;
    else
      return push::f_64::transformers::ProductF64;
  }();
  MockRuntime::reset();
  const auto              tiny = std::numeric_limits<T>::denorm_min();
  T                       received = T{1};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T value) {
    received = value;
    ++calls;
  };
  auto product = Product(0, 15);
  auto output = product({.downstream = std::array{&receive}});
  auto channels = output.channels(2);
  (*channels[0])(tiny);
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 15);
  MockRuntime::tick();
  CHECK_EQ(calls, 0);
  (*channels[1])(T{1});
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(received, tiny);
  (*channels[1])(-tiny);
  MockRuntime::tick();
  CHECK_EQ(calls, 2);
  CHECK_EQ(received, T{0});
  (*channels[0])(-tiny);
  (*channels[1])(T{1});
  MockRuntime::tick();
  CHECK_EQ(calls, 3);
  CHECK_EQ(received, -tiny);
  MockRuntime::close();
  MockRuntime::tick();
  CHECK_EQ(calls, 3);
}

TEST_CASE_TEMPLATE("Sum of opposite subnormals publishes zero",
                   T,
                   f32,
                   f64) {
  constexpr auto Sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32;
    else
      return push::f_64::transformers::SumF64;
  }();
  MockRuntime::reset();
  const auto              tiny = std::numeric_limits<T>::denorm_min();
  T                       received = T{1};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T value) {
    received = value;
    ++calls;
  };
  auto sum = Sum(0, 15);
  auto output = sum({.downstream = std::array{&receive}});
  auto channels = output.channels(2);
  (*channels[0])(tiny);
  (*channels[1])(-tiny);
  MockRuntime::start();
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(received, T{0});
  MockRuntime::close();
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
}

TEST_CASE_TEMPLATE("Constant delivers once per pointer and nothing after an empty rewire",
                   T,
                   f32,
                   f64) {
  constexpr auto Constant = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::ConstF32;
    else
      return push::f_64::sources::ConstF64;
  }();
  MockRuntime::reset();
  u32                     calls = 0;
  T                       received = T{0};
  core::function<void(T)> receive = [&](const T value) {
    received = value;
    ++calls;
  };
  auto repeated = Constant(0, T{-2});
  repeated({.downstream = std::array<core::function<void(T)> *, 2>{&receive, &receive}});
  MockRuntime::start();
  CHECK_EQ(calls, 2);
  CHECK_EQ(received, T{-2});
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);

  MockRuntime::reset();
  calls = 0;
  auto cleared = Constant(1, T{5});
  cleared({.downstream = std::array{&receive}});
  cleared({.downstream = {}});
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("GPIO rewire drops a pin and one close releases every listener",
                   T,
                   f32,
                   f64) {
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();
  MockRuntime::reset();
  u32                     first = 0;
  u32                     second = 0;
  T                       firstValue = T{7};
  T                       secondValue = T{7};
  core::function<void(T)> sinkA = [&](const T value) {
    firstValue = value;
    ++first;
  };
  core::function<void(T)> sinkB = [&](const T value) {
    secondValue = value;
    ++second;
  };
  auto gpio = Gpio(1, 4, core::array<u8>{{u8{1}, u8{2}}});
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{&sinkA},
                                                                        std::array{&sinkB}}}});
  CHECK_EQ(MockRuntime::activeGpioCount(), 2);
  MockRuntime::emitGpio(4, 1, true);
  MockRuntime::emitGpio(4, 2, false);
  CHECK_EQ(first, 1);
  CHECK_EQ(firstValue, T{1});
  CHECK_EQ(second, 1);
  CHECK_EQ(secondValue, T{0});

  const auto deliveredToFirst = first;
  const auto deliveredToSecond = second;
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{&sinkA}}}});
  MockRuntime::emitGpio(4, 2, true);
  CHECK_EQ(second, deliveredToSecond);
  CHECK_EQ(secondValue, T{0});
  MockRuntime::emitGpio(4, 1, false);
  CHECK_GT(first, deliveredToFirst);
  CHECK_EQ(firstValue, T{0});

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  const auto afterClose = first;
  MockRuntime::emitGpio(4, 1, true);
  MockRuntime::emitGpio(4, 2, true);
  CHECK_EQ(first, afterClose);
  CHECK_EQ(second, deliveredToSecond);
}

TEST_CASE("GPIO registration reports an empty pin list") {
  MockRuntime::reset();
  auto gpio = push::f_32::sources::GpioInF32(4, 9, core::array<u8>{});
  CHECK_EQ(MockRuntime::registeredGpioCount(), 1);
  CHECK_EQ(MockRuntime::registeredGpioPort(4), 9);
  CHECK_EQ(MockRuntime::registeredGpioPinCount(4), 0);
  gpio({.pins = {}});
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("Close from a pulse sample or GPIO edge stops later callbacks",
                   T,
                   f32,
                   f64) {
  constexpr auto Pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32;
    else
      return push::f_64::sources::PulseGenF64;
  }();
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();

  MockRuntime::reset();
  u32                     pulses = 0;
  core::function<void(T)> receive = [&](const T value) {
    CHECK_EQ(value, T{4});
    ++pulses;
    MockRuntime::close();
  };
  auto pulse = Pulse(0, T{1}, T{4}, T{0}, T{0});
  pulse({.downstream = std::array{&receive}});
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::tick();
  CHECK_EQ(pulses, 1);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(pulses, 1);

  MockRuntime::reset();
  u32                     edges = 0;
  core::function<void(T)> edge = [&](const T value) {
    CHECK_EQ(value, T{1});
    ++edges;
    MockRuntime::close();
  };
  auto gpio = Gpio(1, 3, core::array<u8>(1, u8{5}));
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{&edge}}}});
  CHECK_EQ(MockRuntime::activeGpioCount(), 1);
  MockRuntime::emitGpio(3, 5, true);
  CHECK_EQ(edges, 1);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::emitGpio(3, 5, false);
  CHECK_EQ(edges, 1);
}
