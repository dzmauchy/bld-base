#include <array>
#include <doctest/doctest.h>

#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <cmath>
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

TEST_CASE_TEMPLATE("Separate GPIO blocks on one pin stay independent when one closes",
                   T,
                   f32,
                   f64) {
  constexpr auto Scope = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32;
    else
      return push::f_64::sinks::ScopeF64;
  }();
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();
  const auto has = [](u32 blockId, u8 channel) {
    if constexpr (std::is_same_v<T, f32>)
      return MockRuntime::hasF32(blockId, channel);
    else
      return MockRuntime::hasF64(blockId, channel);
  };
  const auto last = [](u32 blockId, u8 channel) {
    if constexpr (std::is_same_v<T, f32>)
      return MockRuntime::lastF32(blockId, channel);
    else
      return MockRuntime::lastF64(blockId, channel);
  };

  MockRuntime::reset();
  auto firstScope = Scope(0, 60, 10);
  auto secondScope = Scope(1, 60, 10);
  auto first = Gpio(2, 4, core::array<u8>(1, u8{6}));
  auto second = Gpio(3, 4, core::array<u8>(1, u8{6}));
  auto firstChannels = firstScope().channels(1);
  auto secondChannels = secondScope().channels(1);
  first({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{firstChannels}}});
  second({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{secondChannels}}});

  MockRuntime::emitGpio(4, 6, true);
  CHECK(has(0, 0));
  CHECK(has(1, 0));
  CHECK_EQ(last(0, 0), T{1});
  CHECK_EQ(last(1, 0), T{1});
  MockRuntime::emitGpio(5, 6, false);
  MockRuntime::emitGpio(4, 7, false);
  CHECK_EQ(last(0, 0), T{1});
  CHECK_EQ(last(1, 0), T{1});

  CHECK_EQ(MockRuntime::activeGpioCount(), 2);
  MockRuntime::closeAt(0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 1);
  MockRuntime::emitGpio(4, 6, false);
  CHECK_EQ(last(0, 0), T{1});
  CHECK_EQ(last(1, 0), T{0});
  MockRuntime::closeAt(1);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::emitGpio(4, 6, true);
  CHECK_EQ(last(0, 0), T{1});
  CHECK_EQ(last(1, 0), T{0});
}

TEST_CASE("Scope precisions do not share observations or arm timers") {
  MockRuntime::reset();
  auto f32Scope = push::f_32::sinks::ScopeF32(4);
  auto f64Scope = push::f_64::sinks::ScopeF64(4);
  auto f32Channels = f32Scope().channels(2);
  auto f64Channels = f64Scope().channels(2);
  (*f32Channels[0])(1.5f);
  CHECK(MockRuntime::hasF32(4, 0));
  CHECK_FALSE(MockRuntime::hasF64(4, 0));
  CHECK_FALSE(MockRuntime::hasF32(4, 1));
  (*f64Channels[1])(2.5);
  CHECK_EQ(MockRuntime::lastF32(4, 0), 1.5f);
  CHECK_FALSE(MockRuntime::hasF64(4, 0));
  CHECK(MockRuntime::hasF64(4, 1));
  CHECK_EQ(MockRuntime::lastF64(4, 1), 2.5);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("Unary transformers publish once and do not arm an interval",
                   T,
                   f32,
                   f64) {
  constexpr auto Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SinF32;
    else
      return push::f_64::transformers::SinF64;
  }();
  MockRuntime::reset();
  u32                     calls = 0;
  T                       received = 1;
  core::function<void(T)> sink = [&](const T value) {
    received = value;
    ++calls;
  };
  auto sine = Sine(0);
  auto consumer = sine({.downstream = std::array{&sink}}).consumer;
  REQUIRE(consumer != nullptr);
  (*consumer)(T{0});
  CHECK_EQ(calls, 1);
  CHECK_EQ(received, T{0});
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::start();
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("A huge sine frequency overflows f32 angle math and still wraps in f64",
                   T,
                   f32,
                   f64) {
  constexpr auto Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32;
    else
      return push::f_64::sources::SinGenF64;
  }();
  MockRuntime::reset();
  T                       received = 0;
  bool                    called = false;
  core::function<void(T)> sink = [&](const T value) {
    received = value;
    called = true;
  };
  auto sine = Sine(0, 10, T{1e38}, T{1}, T{0});
  sine({.downstream = std::array{&sink}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::setNow(1000);
  MockRuntime::tick();
  CHECK(called);
  if constexpr (std::is_same_v<T, f32>)
    CHECK(std::isnan(received));
  else {
    CHECK(std::isfinite(received));
    CHECK(std::fabs(received) <= 1);
  }
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}
