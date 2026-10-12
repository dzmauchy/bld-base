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

TEST_CASE_TEMPLATE("A zero amplitude sine publishes exact zero as time advances",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       received = T{99};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T value) {
    received = value;
    ++calls;
  };
  auto sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32(0, 10, T{1}, T{0}, T{0});
    else
      return push::f_64::sources::SinGenF64(0, 10, T{1}, T{0}, T{0});
  }();
  sine({.downstream = std::array{&receive}});
  MockRuntime::start();
  for (const u64 time : {u64{0}, u64{250}}) {
    MockRuntime::setNow(time);
    MockRuntime::tick();
    CHECK_EQ(received, T{0});
  }
  CHECK_EQ(calls, 2);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("A negative pulse frequency inverts the high window",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  constexpr T             amplitude = T{4};
  T                       received = T{99};
  core::function<void(T)> receive = [&](const T value) { received = value; };
  auto                    pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(0, T{0.5}, amplitude, T{-1}, T{0});
    else
      return push::f_64::sources::PulseGenF64(0, T{0.5}, amplitude, T{-1}, T{0});
  }();
  pulse({.downstream = std::array{&receive}});
  MockRuntime::start();
  for (const u64 time : {u64{0}, u64{250}, u64{750}}) {
    MockRuntime::setNow(time);
    MockRuntime::tick();
    CHECK_EQ(received, time == 250 ? T{0} : amplitude);
  }
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("A negative pulse duty stays low through the high half of the period",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  constexpr T             amplitude = T{4};
  T                       received = amplitude;
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T value) {
    received = value;
    ++calls;
  };
  auto pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(0, T{-0.25}, amplitude, T{1}, T{0});
    else
      return push::f_64::sources::PulseGenF64(0, T{-0.25}, amplitude, T{1}, T{0});
  }();
  pulse({.downstream = std::array{&receive}});
  MockRuntime::start();
  for (const u64 time : {u64{0}, u64{100}, u64{250}}) {
    MockRuntime::setNow(time);
    MockRuntime::tick();
    CHECK_EQ(received, T{0});
  }
  CHECK_EQ(calls, 3);
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Sine publishes the NaN result of an infinite input",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  bool                    delivered = false;
  T                       received = T{1};
  core::function<void(T)> receive = [&](const T value) {
    delivered = true;
    received = value;
  };
  auto sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SinF32(0);
    else
      return push::f_64::transformers::SinF64(0);
  }();
  auto input = sine({.downstream = std::array{&receive}}).consumer;
  REQUIRE(input != nullptr);
  (*input)(std::numeric_limits<T>::infinity());
  CHECK(delivered);
  CHECK(std::isnan(received));
  delivered = false;
  (*input)(T{0});
  CHECK(delivered);
  CHECK_EQ(received, T{0});
  MockRuntime::close();
}
