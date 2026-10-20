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

TEST_CASE_TEMPLATE("A periodic source samples once and publishes that value to every sink",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       first = 99;
  T                       second = 99;
  core::function<void(T)> early = [&first](const T value) {
    first = value;
    MockRuntime::setNow(250);
  };
  core::function<void(T)> later = [&second](const T value) { second = value; };
  auto                    sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32(0, 10, T{1}, T{1}, T{0});
    else
      return push::f_64::sources::SinGenF64(0, 10, T{1}, T{1}, T{0});
  }();
  sine({.downstream = std::array{&early, &later}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::tick();
  CHECK_EQ(first, T{0});
  CHECK_EQ(second, T{0});
  MockRuntime::tick();
  CHECK(first == doctest::Approx(1));
  CHECK(second == doctest::Approx(1));
  CHECK_EQ(first, second);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("Wave amplitude of NaN or infinity is forwarded from the sampled angle",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  const auto infinity = std::numeric_limits<T>::infinity();
  T          sineAtZero = 0;
  T          sineLater = 0;
  T          cosinePositive = 0;
  T          cosineNegative = 0;
  T          sineNan = 0;
  core::function<void(T)> receiveSine = [&sineAtZero](const T value) { sineAtZero = value; };
  core::function<void(T)> receiveSineLater = [&sineLater](const T value) { sineLater = value; };
  core::function<void(T)> receivePositive = [&cosinePositive](const T value) {
    cosinePositive = value;
  };
  core::function<void(T)> receiveNegative = [&cosineNegative](const T value) {
    cosineNegative = value;
  };
  core::function<void(T)> receiveNan = [&sineNan](const T value) { sineNan = value; };
  auto                    sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32;
    else
      return push::f_64::sources::SinGenF64;
  }();
  auto cosine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::CosGenF32;
    else
      return push::f_64::sources::CosGenF64;
  }();
  auto positiveSine = sine(0, 10, T{1}, infinity, T{0});
  auto laterSine = sine(1, 10, T{1}, infinity, T{0});
  auto positiveCosine = cosine(2, 10, T{1}, infinity, T{0});
  auto negativeCosine = cosine(3, 10, T{1}, -infinity, T{0});
  auto nanSine = sine(4, 10, T{1}, std::numeric_limits<T>::quiet_NaN(), T{0});
  positiveSine({.downstream = std::array{&receiveSine}});
  laterSine({.downstream = std::array{&receiveSineLater}});
  positiveCosine({.downstream = std::array{&receivePositive}});
  negativeCosine({.downstream = std::array{&receiveNegative}});
  nanSine({.downstream = std::array{&receiveNan}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  MockRuntime::tick();
  CHECK(std::isnan(sineAtZero));
  CHECK_EQ(cosinePositive, infinity);
  CHECK_EQ(cosineNegative, -infinity);
  CHECK(std::isnan(sineNan));
  MockRuntime::setNow(250);
  MockRuntime::tick();
  CHECK_EQ(sineLater, infinity);
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Pulse negative zero stays negative only while the duty window is high",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       value = 1;
  core::function<void(T)> receive = [&value](const T sample) { value = sample; };
  auto                    pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(0, T{0.5}, -T{0}, T{1}, T{0});
    else
      return push::f_64::sources::PulseGenF64(0, T{0.5}, -T{0}, T{1}, T{0});
  }();
  pulse({.downstream = std::array{&receive}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  MockRuntime::tick();
  CHECK_EQ(value, T{0});
  CHECK(std::signbit(value));
  MockRuntime::setNow(500);
  MockRuntime::tick();
  CHECK_EQ(value, T{0});
  CHECK_FALSE(std::signbit(value));
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Aggregate fan-out finishes the current sample when a sink closes",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       second = 0;
  u32                     secondCalls = 0;
  core::function<void(T)> closing = [](const T) { MockRuntime::close(); };
  core::function<void(T)> later = [&](const T value) {
    second = value;
    ++secondCalls;
  };
  auto sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32(1, 10);
    else
      return push::f_64::transformers::SumF64(1, 10);
  }();
  auto channels = sum({.downstream = std::array{&closing, &later}}).channels(2);
  (*channels[0])(T{3});
  (*channels[1])(T{4});
  MockRuntime::start();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  MockRuntime::tick();
  CHECK_EQ(second, T{7});
  CHECK_EQ(secondCalls, 1);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(secondCalls, 1);
}
