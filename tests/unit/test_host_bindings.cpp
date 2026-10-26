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

TEST_CASE_TEMPLATE("A half-turn pulse phase stays low until elapsed time wraps it high",
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
      return push::f_32::sources::PulseGenF32(0, T{0.5}, T{4}, T{1}, std::numbers::pi_v<T>);
    else
      return push::f_64::sources::PulseGenF64(0, T{0.5}, T{4}, T{1}, std::numbers::pi_v<T>);
  }();
  pulse({.downstream = std::array{&receive}});
  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(calls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);

  const u64 times[]{0, 250, 500, 750, 1250};
  const T   expected[]{T{0}, T{0}, T{4}, T{4}, T{0}};
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
  MockRuntime::setNow(1750);
  MockRuntime::tick();
  CHECK_EQ(calls, callsAtClose);
  CHECK_EQ(value, valueAtClose);
}

TEST_CASE_TEMPLATE("Unary sine and cosine replace infinities with NaN and keep later finite samples",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
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
  const auto              infinity = std::numeric_limits<T>::infinity();
  T                       sineA = T{3};
  T                       sineB = T{3};
  T                       cosineA = T{3};
  T                       cosineB = T{3};
  u32                     sineCalls = 0;
  u32                     cosineCalls = 0;
  core::function<void(T)> receiveSineA = [&](const T sample) {
    sineA = sample;
    ++sineCalls;
  };
  core::function<void(T)> receiveSineB = [&](const T sample) {
    sineB = sample;
    ++sineCalls;
  };
  core::function<void(T)> receiveCosineA = [&](const T sample) {
    cosineA = sample;
    ++cosineCalls;
  };
  core::function<void(T)> receiveCosineB = [&](const T sample) {
    cosineB = sample;
    ++cosineCalls;
  };
  auto       sine = Sine(0);
  auto       cosine = Cosine(1);
  const auto sineInput = sine({.downstream = std::array{&receiveSineA, &receiveSineB}}).consumer;
  const auto cosineInput =
      cosine({.downstream = std::array{&receiveCosineA, &receiveCosineB}}).consumer;
  REQUIRE(sineInput != nullptr);
  REQUIRE(cosineInput != nullptr);

  (*sineInput)(infinity);
  CHECK(std::isnan(sineA));
  CHECK(std::isnan(sineB));
  CHECK_FALSE(std::isinf(sineA));
  CHECK_EQ(sineCalls, 2);
  (*sineInput)(-infinity);
  CHECK(std::isnan(sineA));
  CHECK(std::isnan(sineB));
  CHECK_EQ(sineCalls, 4);
  (*sineInput)(T{0});
  CHECK_EQ(sineA, T{0});
  CHECK_EQ(sineB, T{0});
  CHECK_FALSE(std::signbit(sineA));
  CHECK_EQ(sineCalls, 6);

  (*cosineInput)(-infinity);
  CHECK(std::isnan(cosineA));
  CHECK(std::isnan(cosineB));
  CHECK_FALSE(std::isinf(cosineA));
  CHECK_EQ(cosineCalls, 2);
  (*cosineInput)(infinity);
  CHECK(std::isnan(cosineA));
  CHECK(std::isnan(cosineB));
  CHECK_EQ(cosineCalls, 4);
  (*cosineInput)(T{0});
  CHECK_EQ(cosineA, T{1});
  CHECK_EQ(cosineB, T{1});
  CHECK_EQ(cosineCalls, 6);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("Unary fan-out delivers the sample computed before a sink reenters",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  constexpr auto Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SinF32;
    else
      return push::f_64::transformers::SinF64;
  }();
  core::function<void(T)> *input = nullptr;
  T                        firstSamples[2]{T{7}, T{7}};
  T                        secondSamples[2]{T{7}, T{7}};
  u32                      firstCalls = 0;
  u32                      secondCalls = 0;
  bool                     reentered = false;
  core::function<void(T)>  first = [&](const T sample) {
    REQUIRE(firstCalls < 2);
    firstSamples[firstCalls++] = sample;
    if (!reentered) {
      reentered = true;
      (*input)(std::numbers::pi_v<T> / T{2});
    }
  };
  core::function<void(T)> second = [&](const T sample) {
    REQUIRE(secondCalls < 2);
    secondSamples[secondCalls++] = sample;
  };
  auto sine = Sine(0);
  input = sine({.downstream = std::array{&first, &second}}).consumer;
  REQUIRE(input != nullptr);
  (*input)(T{0});

  CHECK_EQ(firstCalls, 2);
  CHECK_EQ(secondCalls, 2);
  CHECK_EQ(firstSamples[0], T{0});
  CHECK_FALSE(std::signbit(firstSamples[0]));
  CHECK(firstSamples[1] == doctest::Approx(1));
  CHECK(secondSamples[0] == doctest::Approx(1));
  CHECK_EQ(secondSamples[1], T{0});
  CHECK_FALSE(std::signbit(secondSamples[1]));
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE("A single latched negative zero is published without an identity term",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  constexpr auto Sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32;
    else
      return push::f_64::transformers::SumF64;
  }();
  constexpr auto Product = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::ProductF32;
    else
      return push::f_64::transformers::ProductF64;
  }();
  const T                 negativeZero = -T{0};
  T                       sumValue = T{5};
  T                       productValue = T{5};
  u32                     sumCalls = 0;
  u32                     productCalls = 0;
  core::function<void(T)> receiveSum = [&](const T sample) {
    sumValue = sample;
    ++sumCalls;
  };
  core::function<void(T)> receiveProduct = [&](const T sample) {
    productValue = sample;
    ++productCalls;
  };
  auto sum = Sum(0, 10);
  auto product = Product(1, 10);
  auto sumOutput = sum({.downstream = std::array{&receiveSum}});
  auto productOutput = product({.downstream = std::array{&receiveProduct}});
  auto term = sumOutput.channels(1);
  auto factor = productOutput.channels(1);
  REQUIRE_EQ(term.size(), 1);
  REQUIRE_EQ(factor.size(), 1);
  (*term[0])(negativeZero);
  (*factor[0])(negativeZero);

  MockRuntime::start();
  CHECK_EQ(sumCalls, 0);
  CHECK_EQ(productCalls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 2);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 10);
  CHECK_EQ(MockRuntime::intervalPeriodAt(1), 10);
  MockRuntime::tick();
  CHECK_EQ(sumCalls, 1);
  CHECK_EQ(productCalls, 1);
  CHECK(sumValue == T{0});
  CHECK(std::signbit(sumValue));
  CHECK(productValue == T{0});
  CHECK(std::signbit(productValue));

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  MockRuntime::tick();
  CHECK_EQ(sumCalls, 1);
  CHECK_EQ(productCalls, 1);
}
