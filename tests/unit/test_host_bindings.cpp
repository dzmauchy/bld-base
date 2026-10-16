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

TEST_CASE_TEMPLATE("Pulse measures its period from runtime start",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       received = T{0};
  u32                     calls = 0;
  core::function<void(T)> sink = [&](const T value) {
    received = value;
    ++calls;
  };
  constexpr auto Pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32;
    else
      return push::f_64::sources::PulseGenF64;
  }();
  // 600 ms is already in the low half of a 1 Hz period. Elapsed time is zero
  // until start, so the first sample stays high.
  auto pulse = Pulse(1, T{0.5}, T{4}, T{1}, T{0});
  MockRuntime::setNow(600);
  pulse({.downstream = std::array{&sink}});
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(calls, 0);

  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(received, T{4});

  MockRuntime::setNow(1099);
  MockRuntime::tick();
  CHECK_EQ(calls, 2);
  CHECK_EQ(received, T{4});

  MockRuntime::setNow(1101);
  MockRuntime::tick();
  CHECK_EQ(calls, 3);
  CHECK_EQ(received, T{0});
  CHECK_FALSE(std::signbit(received));

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(calls, 3);
}

TEST_CASE_TEMPLATE("Pulse forwards non-finite amplitudes only while the signal is high",
                   T,
                   f32,
                   f64) {
  auto sample = [](const T amplitude, const u64 time) {
    MockRuntime::reset();
    T                       received = T{1};
    u32                     calls = 0;
    core::function<void(T)> sink = [&](const T value) {
      received = value;
      ++calls;
    };
    constexpr auto Pulse = [] {
      if constexpr (std::is_same_v<T, f32>)
        return push::f_32::sources::PulseGenF32;
      else
        return push::f_64::sources::PulseGenF64;
    }();
    auto pulse = Pulse(1, T{0.5}, amplitude, T{1}, T{0});
    pulse({.downstream = std::array{&sink}});
    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::setNow(time);
    MockRuntime::tick();
    REQUIRE_EQ(calls, 1);
    return received;
  };

  const auto nan = std::numeric_limits<T>::quiet_NaN();
  CHECK(std::isnan(sample(nan, 0)));
  const auto nanLow = sample(nan, 750);
  CHECK_EQ(nanLow, T{0});
  CHECK_FALSE(std::signbit(nanLow));

  const auto positive = std::numeric_limits<T>::infinity();
  CHECK(sample(positive, 0) == positive);
  const auto positiveLow = sample(positive, 750);
  CHECK_EQ(positiveLow, T{0});
  CHECK_FALSE(std::signbit(positiveLow));

  const auto negative = -std::numeric_limits<T>::infinity();
  CHECK(sample(negative, 0) == negative);
  const auto negativeLow = sample(negative, 750);
  CHECK_EQ(negativeLow, T{0});
  CHECK_FALSE(std::signbit(negativeLow));
}

TEST_CASE_TEMPLATE("A finite product publishes again after an overflowing partial",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       received = T{0};
  u32                     calls = 0;
  core::function<void(T)> sink = [&](const T value) {
    received = value;
    ++calls;
  };
  constexpr auto Product = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::ProductF32;
    else
      return push::f_64::transformers::ProductF64;
  }();
  const auto big = std::ldexp(T{1}, std::is_same_v<T, f32> ? 100 : 1000);
  const auto small = std::ldexp(T{1}, std::is_same_v<T, f32> ? -50 : -500);
  REQUIRE(std::isfinite(big));
  REQUIRE(std::isfinite(small));
  REQUIRE_FALSE(std::isfinite(big * big));
  const auto finite = big * small;
  REQUIRE(std::isfinite(finite));

  auto product = Product(1, 10);
  auto output = product({.downstream = std::array{&sink}});
  auto inputs = output.channels(2);
  (*inputs[0])(big);
  (*inputs[1])(big);
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::tick();
  CHECK_EQ(calls, 0);

  (*inputs[1])(small);
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(received, finite);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("Rewiring GPIO input replaces the previous consumer group",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       first = T{-1};
  T                       second = T{-1};
  u32                     firstCalls = 0;
  u32                     secondCalls = 0;
  core::function<void(T)> firstSink = [&](const T value) {
    first = value;
    ++firstCalls;
  };
  core::function<void(T)> secondSink = [&](const T value) {
    second = value;
    ++secondCalls;
  };
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();
  auto gpio = Gpio(1, 9, core::array<u8>(1, u8{4}));
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{&firstSink}}}});
  MockRuntime::emitGpio(9, 4, true);
  CHECK_EQ(first, T{1});
  CHECK_EQ(firstCalls, 1);
  CHECK_EQ(secondCalls, 0);

  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{&secondSink}}}});
  CHECK_GT(MockRuntime::activeGpioCount(), 0);
  MockRuntime::emitGpio(9, 4, false);
  CHECK_EQ(firstCalls, 1);
  CHECK_EQ(second, T{0});
  CHECK_GT(secondCalls, 0);
  const auto deliveries = secondCalls;

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::emitGpio(9, 4, true);
  CHECK_EQ(firstCalls, 1);
  CHECK_EQ(secondCalls, deliveries);
  CHECK_EQ(second, T{0});
}
