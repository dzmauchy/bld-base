#include <array>
#include <cmath>
#include <doctest/doctest.h>

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

TEST_CASE_TEMPLATE("A quarter duty pulse falls at 250 ms and restarts on the next period",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       value = T{9};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T sample) {
    value = sample;
    ++calls;
  };
  auto pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(0, T{0.25}, T{4}, T{1}, T{0});
    else
      return push::f_64::sources::PulseGenF64(0, T{0.25}, T{4}, T{1}, T{0});
  }();
  pulse({.downstream = std::array{&receive}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);

  const u64 times[]{0, 249, 250, 500, 1000};
  const T   expected[]{T{4}, T{4}, T{0}, T{0}, T{4}};
  for (u32 index = 0; index < 5; ++index) {
    MockRuntime::setNow(times[index]);
    MockRuntime::tick();
    CHECK_EQ(value, expected[index]);
    CHECK_FALSE(std::signbit(value));
  }
  CHECK_EQ(calls, 5);

  const auto callsAtClose = calls;
  const auto valueAtClose = value;
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::setNow(249);
  MockRuntime::tick();
  CHECK_EQ(calls, callsAtClose);
  CHECK_EQ(value, valueAtClose);
}

TEST_CASE_TEMPLATE("Negative sine amplitude flips the peaks and keeps a signed zero",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       value = T{9};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T sample) {
    value = sample;
    ++calls;
  };
  auto sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32(0, 10, T{1}, T{-2}, T{0});
    else
      return push::f_64::sources::SinGenF64(0, 10, T{1}, T{-2}, T{0});
  }();
  sine({.downstream = std::array{&receive}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 10);

  const u64  times[]{0, 250, 750, 1000};
  const T    expected[]{T{0}, T{-2}, T{2}, T{0}};
  const bool negative[]{true, true, false, true};
  for (u32 index = 0; index < 4; ++index) {
    MockRuntime::setNow(times[index]);
    MockRuntime::tick();
    CHECK_EQ(value, expected[index]);
    CHECK_EQ(std::signbit(value), negative[index]);
  }
  CHECK_EQ(calls, 4);

  const auto callsAtClose = calls;
  const auto valueAtClose = value;
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::setNow(250);
  MockRuntime::tick();
  CHECK_EQ(calls, callsAtClose);
  CHECK_EQ(value, valueAtClose);
  CHECK(std::signbit(value));
}

TEST_CASE_TEMPLATE("A zero factor zeroes the whole product until that factor is replaced",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       value = T{9};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](const T sample) {
    value = sample;
    ++calls;
  };
  constexpr auto Product = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::ProductF32;
    else
      return push::f_64::transformers::ProductF64;
  }();
  auto product = Product(0, 10);
  auto output = product({.downstream = std::array{&receive}});
  auto factors = output.channels(3);
  REQUIRE_EQ(factors.size(), 3);
  (*factors[0])(T{4});
  (*factors[2])(T{5});

  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 10);
  MockRuntime::tick();
  CHECK_EQ(calls, 0);

  (*factors[1])(T{0});
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(value, T{0});
  CHECK_FALSE(std::signbit(value));

  (*factors[1])(T{2});
  MockRuntime::tick();
  CHECK_EQ(calls, 2);
  CHECK_EQ(value, T{40});

  const auto callsAtClose = calls;
  const auto valueAtClose = value;
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  (*factors[1])(T{3});
  MockRuntime::tick();
  CHECK_EQ(calls, callsAtClose);
  CHECK_EQ(value, valueAtClose);
}
