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

TEST_CASE_TEMPLATE("GPIO fan-out keeps the level sampled before a sink changes the pin",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T   earlySamples[2]{};
  T   laterSamples[2]{};
  u32 earlyCalls = 0;
  u32 laterCalls = 0;
  auto gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32(1, 4, core::array<u8>(1, u8{2}));
    else
      return push::f_64::sources::GpioInF64(1, 4, core::array<u8>(1, u8{2}));
  }();
  core::function<void(T)> early = [&](const T value) {
    if (earlyCalls < 2)
      earlySamples[earlyCalls] = value;
    if (++earlyCalls == 1)
      MockRuntime::emitGpio(4, 2, false);
  };
  core::function<void(T)> later = [&](const T value) {
    if (laterCalls < 2)
      laterSamples[laterCalls] = value;
    ++laterCalls;
  };
  gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{
            {std::array{&early, &later}}}});

  CHECK_EQ(MockRuntime::activeGpioCount(), 1);
  MockRuntime::emitGpio(4, 2, true);
  REQUIRE_EQ(earlyCalls, 2);
  REQUIRE_EQ(laterCalls, 2);
  CHECK_EQ(earlySamples[0], T{1});
  CHECK_EQ(earlySamples[1], T{0});
  CHECK_EQ(laterSamples[0], T{0});
  CHECK_EQ(laterSamples[1], T{1});
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  MockRuntime::emitGpio(4, 2, true);
  CHECK_EQ(earlyCalls, 2);
  CHECK_EQ(laterCalls, 2);
}

TEST_CASE("Random generators read the host sample for their precision") {
  MockRuntime::reset();
  MockRuntime::setRandom(0.25f);
  MockRuntime::setRandomF64(0.5);
  CHECK_EQ(random_f32(), 0.25f);
  CHECK_EQ(random_f64(), 0.5);

  f32                       narrow = 0;
  f64                       wide = 0;
  core::function<void(f32)> receiveNarrow = [&](const f32 value) { narrow = value; };
  core::function<void(f64)> receiveWide = [&](const f64 value) { wide = value; };
  auto                      f32Random = push::f_32::sources::RandGenF32(0, 10, 2.f);
  auto                      f64Random = push::f_64::sources::RandGenF64(1, 10, 4.0);
  f32Random({.downstream = std::array{&receiveNarrow}});
  f64Random({.downstream = std::array{&receiveWide}});
  MockRuntime::start();
  CHECK_EQ(narrow, 0.f);
  CHECK_EQ(wide, 0.0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 2);
  MockRuntime::tick();
  CHECK_EQ(narrow, 0.5f);
  CHECK_EQ(wide, 2.0);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(narrow, 0.5f);
  CHECK_EQ(wide, 2.0);
}
