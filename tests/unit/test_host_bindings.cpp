#include <array>
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

TEST_CASE_TEMPLATE("A barely negative pulse phase wraps to the low part of the cycle",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T   samples[3]{};
  u32 calls = 0;
  core::function<void(T)> receive = [&](const T value) {
    if (calls < 3)
      samples[calls] = value;
    ++calls;
  };
  constexpr T phase = -T{1} / T{1024};
  constexpr T amplitude = T{4};
  auto        pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(0, T{0.5}, amplitude, T{1}, phase);
    else
      return push::f_64::sources::PulseGenF64(0, T{0.5}, amplitude, T{1}, phase);
  }();
  pulse({.downstream = std::array{&receive}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);
  MockRuntime::tick();
  MockRuntime::setNow(1);
  MockRuntime::tick();
  MockRuntime::setNow(501);
  MockRuntime::tick();
  REQUIRE_EQ(calls, 3);
  CHECK_EQ(samples[0], T{0});
  CHECK_EQ(samples[1], amplitude);
  CHECK_EQ(samples[2], T{0});
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(calls, 3);
}

TEST_CASE_TEMPLATE("A full-turn phase is reduced before the wave is sampled",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T   sineValue = T{7};
  T   cosineValue = T{7};
  u32 sineCalls = 0;
  u32 cosineCalls = 0;
  core::function<void(T)> receiveSine = [&](const T value) {
    sineValue = value;
    ++sineCalls;
  };
  core::function<void(T)> receiveCosine = [&](const T value) {
    cosineValue = value;
    ++cosineCalls;
  };
  constexpr T phase = T{2} * core::pi_v<T>;
  auto        sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32(0, 10, T{1}, T{1}, phase);
    else
      return push::f_64::sources::SinGenF64(0, 10, T{1}, T{1}, phase);
  }();
  auto cosine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::CosGenF32(1, 10, T{1}, T{1}, phase);
    else
      return push::f_64::sources::CosGenF64(1, 10, T{1}, T{1}, phase);
  }();
  sine({.downstream = std::array{&receiveSine}});
  cosine({.downstream = std::array{&receiveCosine}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(sineCalls, 0);
  CHECK_EQ(cosineCalls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 2);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 10);
  CHECK_EQ(MockRuntime::intervalPeriodAt(1), 10);
  MockRuntime::tick();
  CHECK_EQ(sineCalls, 1);
  CHECK_EQ(cosineCalls, 1);
  CHECK_EQ(sineValue, T{0});
  CHECK_EQ(cosineValue, T{1});
  MockRuntime::setNow(250);
  MockRuntime::tick();
  CHECK_EQ(sineCalls, 2);
  CHECK_EQ(cosineCalls, 2);
  CHECK_EQ(sineValue, T{1});
  const T cosineMagnitude = cosineValue < T{0} ? -cosineValue : cosineValue;
  CHECK(cosineMagnitude < T{1e-5});
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(sineCalls, 2);
  CHECK_EQ(cosineCalls, 2);
}
