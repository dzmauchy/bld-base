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

TEST_CASE_TEMPLATE("Pulse reduces phases beyond one turn and holds a zero frequency",
                   T,
                   f32,
                   f64) {
  constexpr auto amplitude = T{3};
  constexpr auto beyondOneTurn = std::numbers::pi_v<T> * T{4.5};
  constexpr auto lowPhase = std::numbers::pi_v<T> * T{1.5};
  auto           pulse = [](u32 id,
                            T   frequency,
                            T   phase) {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32(id, T{0.5}, amplitude, frequency, phase);
    else
      return push::f_64::sources::PulseGenF64(id, T{0.5}, amplitude, frequency, phase);
  };

  MockRuntime::reset();
  T                       positive = 0;
  T                       negative = 0;
  T                       frozenHigh = 0;
  T                       frozenLow = 0;
  core::function<void(T)> receivePositive = [&](const T value) { positive = value; };
  core::function<void(T)> receiveNegative = [&](const T value) { negative = value; };
  core::function<void(T)> receiveFrozenHigh = [&](const T value) { frozenHigh = value; };
  core::function<void(T)> receiveFrozenLow = [&](const T value) { frozenLow = value; };
  auto                    positivePulse = pulse(0, T{1}, beyondOneTurn);
  auto                    negativePulse = pulse(1, T{1}, -beyondOneTurn);
  auto                    highPulse = pulse(2, T{0}, T{0});
  auto                    lowPulse = pulse(3, T{0}, lowPhase);
  positivePulse({.downstream = std::array{&receivePositive}});
  negativePulse({.downstream = std::array{&receiveNegative}});
  highPulse({.downstream = std::array{&receiveFrozenHigh}});
  lowPulse({.downstream = std::array{&receiveFrozenLow}});

  MockRuntime::setNow(0);
  MockRuntime::start();
  MockRuntime::tick();
  CHECK_EQ(positive, amplitude);
  CHECK_EQ(negative, T{0});
  CHECK_EQ(frozenHigh, amplitude);
  CHECK_EQ(frozenLow, T{0});

  MockRuntime::setNow(500);
  MockRuntime::tick();
  CHECK_EQ(positive, T{0});
  CHECK_EQ(negative, amplitude);
  CHECK_EQ(frozenHigh, amplitude);
  CHECK_EQ(frozenLow, T{0});

  MockRuntime::setNow(5000);
  MockRuntime::tick();
  CHECK_EQ(positive, amplitude);
  CHECK_EQ(negative, T{0});
  CHECK_EQ(frozenHigh, amplitude);
  CHECK_EQ(frozenLow, T{0});
  MockRuntime::close();
}
