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

TEST_CASE_TEMPLATE("Pulse treats an exact number of turns as the start of the period",
                   T,
                   f32,
                   f64) {
  constexpr auto amplitude = T{4};
  constexpr auto twoPi = T{2} * core::pi_v<T>;
  auto           pulse = [](T frequency, T phase) {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(1, T{0.5}, amplitude, frequency, phase);
    else
      return push::f_64::sources::PulseGenF64(1, T{0.5}, amplitude, frequency, phase);
  };

  for (const T turns : {T{1}, T{-1}, T{2}, T{-2}}) {
    CAPTURE(turns);
    T                       value = T{-1};
    u32                     calls = 0;
    core::function<void(T)> receive = [&](T sample) {
      value = sample;
      ++calls;
    };
    MockRuntime::reset();
    auto generator = pulse(T{0}, turns * twoPi);
    generator({.downstream = std::array{&receive}});
    MockRuntime::start();
    for (const u64 time : {u64{0}, u64{500}, u64{5000}}) {
      MockRuntime::setNow(time);
      MockRuntime::tick();
      CHECK_EQ(calls, 1);
      CHECK_EQ(value, amplitude);
      calls = 0;
    }
    MockRuntime::close();
  }

  for (const T turns : {T{-1}, T{2}, T{-2}}) {
    CAPTURE(turns);
    T                       value = T{-1};
    core::function<void(T)> receive = [&](T sample) { value = sample; };
    MockRuntime::reset();
    auto generator = pulse(T{1}, turns * twoPi);
    generator({.downstream = std::array{&receive}});
    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);
    MockRuntime::setNow(0);
    MockRuntime::tick();
    CHECK_EQ(value, amplitude);
    MockRuntime::setNow(500);
    MockRuntime::tick();
    CHECK_EQ(value, T{0});
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }
}

TEST_CASE_TEMPLATE("Pulse publishes a low sample for NaN duty and stays high above one",
                   T,
                   f32,
                   f64) {
  constexpr auto amplitude = T{4};
  auto           pulse = [](T duty) {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(1, duty, amplitude, T{1}, T{0});
    else
      return push::f_64::sources::PulseGenF64(1, duty, amplitude, T{1}, T{0});
  };
  T                       value = amplitude;
  u32                     calls = 0;
  core::function<void(T)> receive = [&](T sample) {
    value = sample;
    ++calls;
  };

  MockRuntime::reset();
  auto unordered = pulse(std::numeric_limits<T>::quiet_NaN());
  unordered({.downstream = std::array{&receive}});
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  for (const u64 time : {u64{0}, u64{250}, u64{500}}) {
    MockRuntime::setNow(time);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(value, T{0});
    calls = 0;
  }

  MockRuntime::reset();
  calls = 0;
  value = T{0};
  auto aboveOne = pulse(T{1.5});
  aboveOne({.downstream = std::array{&receive}});
  MockRuntime::start();
  for (const u64 time : {u64{0}, u64{500}, u64{999}}) {
    MockRuntime::setNow(time);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(value, amplitude);
    calls = 0;
  }
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Random scaling keeps the host sample sign and forwards non-finite products",
                   T,
                   f32,
                   f64) {
  auto generatorFor = [](T amplitude) {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::RandGenF32(1, 40, amplitude);
    else
      return push::f_64::sources::RandGenF64(1, 40, amplitude);
  };
  T                       value = T{0};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](T sample) {
    value = sample;
    ++calls;
  };
  auto sample = [&](T amplitude, f32 random) {
    MockRuntime::reset();
    MockRuntime::setRandom(random);
    calls = 0;
    auto generator = generatorFor(amplitude);
    generator({.downstream = std::array{&receive}});
    MockRuntime::start();
    CHECK_EQ(calls, 0);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 40);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    return value;
  };

  CHECK_EQ(sample(T{4}, -0.25f), T{-1});
  CHECK(std::isnan(sample(std::numeric_limits<T>::quiet_NaN(), 0.5f)));
  const auto positiveInfinity = sample(std::numeric_limits<T>::infinity(), 0.5f);
  CHECK(std::isinf(positiveInfinity));
  CHECK(positiveInfinity > T{0});
  CHECK(std::isnan(sample(std::numeric_limits<T>::infinity(), 0.f)));
}

TEST_CASE_TEMPLATE("A sine generator publishes NaN when its phase is NaN",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       value = T{0};
  u32                     calls = 0;
  core::function<void(T)> receive = [&](T sample) {
    value = sample;
    ++calls;
  };
  const auto phase = std::numeric_limits<T>::quiet_NaN();
  auto       sine = [&] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32(1, 25, T{1}, T{3}, phase);
    else
      return push::f_64::sources::SinGenF64(1, 25, T{1}, T{3}, phase);
  }();
  sine({.downstream = std::array{&receive}});
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK(std::isnan(value));
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("GPIO listeners on one pin all observe edges before start",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T   first = T{-1};
  T   second = T{-1};
  T   other = T{-1};
  u32 firstCalls = 0;
  u32 secondCalls = 0;
  u32 otherCalls = 0;
  core::function<void(T)> receiveFirst = [&](T value) {
    first = value;
    ++firstCalls;
  };
  core::function<void(T)> receiveSecond = [&](T value) {
    second = value;
    ++secondCalls;
  };
  core::function<void(T)> receiveOther = [&](T value) {
    other = value;
    ++otherCalls;
  };
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();
  auto firstBlock = Gpio(1, 7, core::array<u8>(1, u8{3}));
  auto secondBlock = Gpio(2, 7, core::array<u8>(1, u8{3}));
  auto otherBlock = Gpio(3, 7, core::array<u8>(1, u8{4}));
  firstBlock({.pins = core::array<VectorizedInput<core::function<void(T)>>>{
                  {std::array{&receiveFirst}}}});
  secondBlock({.pins = core::array<VectorizedInput<core::function<void(T)>>>{
                   {std::array{&receiveSecond}}}});
  otherBlock(
      {.pins = core::array<VectorizedInput<core::function<void(T)>>>{{std::array{&receiveOther}}}});

  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 3);
  CHECK_EQ(MockRuntime::registeredGpioCount(), 3);
  CHECK_EQ(MockRuntime::registeredGpioPin(1, 0), 3);
  CHECK_EQ(MockRuntime::registeredGpioPin(2, 0), 3);
  CHECK_EQ(MockRuntime::registeredGpioPin(3, 0), 4);

  MockRuntime::emitGpio(7, 3, true);
  CHECK_EQ(first, T{1});
  CHECK_EQ(second, T{1});
  CHECK_EQ(firstCalls, 1);
  CHECK_EQ(secondCalls, 1);
  CHECK_EQ(otherCalls, 0);

  MockRuntime::emitGpio(7, 3, false);
  CHECK_EQ(first, T{0});
  CHECK_EQ(second, T{0});
  CHECK_EQ(firstCalls, 2);
  CHECK_EQ(secondCalls, 2);
  CHECK_EQ(otherCalls, 0);

  MockRuntime::emitGpio(7, 4, true);
  CHECK_EQ(other, T{1});
  CHECK_EQ(otherCalls, 1);
  CHECK_EQ(firstCalls, 2);
  CHECK_EQ(secondCalls, 2);

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::emitGpio(7, 3, true);
  MockRuntime::emitGpio(7, 4, false);
  CHECK_EQ(first, T{0});
  CHECK_EQ(second, T{0});
  CHECK_EQ(other, T{1});
  CHECK_EQ(firstCalls, 2);
  CHECK_EQ(secondCalls, 2);
  CHECK_EQ(otherCalls, 1);
}

TEST_CASE_TEMPLATE("An empty GPIO pin list registers no listeners",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  auto gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32(8, 4, core::array<u8>(0));
    else
      return push::f_64::sources::GpioInF64(8, 4, core::array<u8>(0));
  }();
  CHECK_EQ(MockRuntime::registeredGpioCount(), 1);
  CHECK_EQ(MockRuntime::registeredGpioPort(8), 4);
  CHECK_EQ(MockRuntime::registeredGpioPinCount(8), 0);
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{}});
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::emitGpio(4, 0, true);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}
