#include <array>
#include <doctest/doctest.h>

#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <concepts>
#include <limits>
#include <type_traits>

#include "../mock_runtime.hpp"

namespace {

struct Inputs {
  i32 left;
  i32 right;
};

struct Outputs {
  /**
   * Sum
   * @brief The sum of the two input values.
   * @image sum.svg
   */
  i32 sum;
  /**
   * Difference
   * @brief The first input value minus the second.
   * @image difference.svg
   */
  i32 difference;
};

struct Empty {};
static_assert(std::same_as<decltype(push::f_32::sources::ConstF32(0)),
                           core::function<void(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_32::sources::CosGenF32(0)),
                           core::function<void(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_32::sources::SinGenF32(0)),
                           core::function<void(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_32::sources::RandGenF32(0)),
                           core::function<void(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_32::sources::PulseGenF32(0)),
                           core::function<void(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_32::sources::GpioInF32(0)),
                           core::function<void(push::GpioInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_32::sinks::ScopeF32(0)),
                           core::function<push::f_32::sinks::ScopeF32Output()>>);
static_assert(std::same_as<
              decltype(push::f_32::transformers::CosF32(0)),
              core::function<push::f_32::transformers::CosF32Output(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<
              decltype(push::f_32::transformers::SinF32(0)),
              core::function<push::f_32::transformers::SinF32Output(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<
              decltype(push::f_32::transformers::SumF32(0)),
              core::function<push::f_32::transformers::SumF32Output(push::DownstreamInput<f32>)>>);
static_assert(
    std::same_as<
        decltype(push::f_32::transformers::ProductF32(0)),
        core::function<push::f_32::transformers::ProductF32Output(push::DownstreamInput<f32>)>>);
static_assert(std::same_as<decltype(push::f_64::sources::ConstF64(0)),
                           core::function<void(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<decltype(push::f_64::sources::CosGenF64(0)),
                           core::function<void(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<decltype(push::f_64::sources::SinGenF64(0)),
                           core::function<void(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<decltype(push::f_64::sources::RandGenF64(0)),
                           core::function<void(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<decltype(push::f_64::sources::PulseGenF64(0)),
                           core::function<void(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<decltype(push::f_64::sources::GpioInF64(0)),
                           core::function<void(push::GpioInput<f64>)>>);
static_assert(std::same_as<decltype(push::f_64::sinks::ScopeF64(0)),
                           core::function<push::f_64::sinks::ScopeF64Output()>>);
static_assert(std::same_as<
              decltype(push::f_64::transformers::CosF64(0)),
              core::function<push::f_64::transformers::CosF64Output(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<
              decltype(push::f_64::transformers::SinF64(0)),
              core::function<push::f_64::transformers::SinF64Output(push::DownstreamInput<f64>)>>);
static_assert(std::same_as<
              decltype(push::f_64::transformers::SumF64(0)),
              core::function<push::f_64::transformers::SumF64Output(push::DownstreamInput<f64>)>>);
static_assert(
    std::same_as<
        decltype(push::f_64::transformers::ProductF64(0)),
        core::function<push::f_64::transformers::ProductF64Output(push::DownstreamInput<f64>)>>);

/**
 * DualScopeOutput
 * @brief Two independent output ports, one vectorized and one scalar.
 * @image scope.svg
 */
template <typename T> struct DualScopeOutput {
  using Value = T;

  /**
   * Channels
   * @brief One vectorized output carrying the first scope's consumers.
   * @image scope.svg
   */
  VectorizedOutput<core::function<void(T)>> channels{};

  /**
   * Single
   * @brief A separate output carrying the second scope's consumer.
   * @image scope.svg
   */
  core::function<void(T)> *single{nullptr};
};

template <typename O> core::function<O()> DualScope(u32 blockId) {
  using T = O::Value;
  auto first = [&] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32(blockId);
    else
      return push::f_64::sinks::ScopeF64(blockId);
  }();
  auto second = [&] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32(blockId + 1);
    else
      return push::f_64::sinks::ScopeF64(blockId + 1);
  }();
  return [first, second] {
    return O{.channels = first().channels, .single = second().channels(1)[0]};
  };
}

} // namespace

TEST_CASE("Block dispatches structs with multiple ports") {
  core::function<Outputs(Inputs)> block = [](Inputs input) {
    return Outputs{input.left + input.right, input.left - input.right};
  };
  const auto output = block({.left = 7, .right = 3});
  CHECK_EQ(output.sum, 10);
  CHECK_EQ(output.difference, 4);
}

TEST_CASE("Block supports an empty input struct and void output") {
  u32                         calls = 0;
  core::function<void(Empty)> block = [&calls](Empty) { ++calls; };
  block({});
  CHECK_EQ(calls, 1);
}

TEST_CASE_TEMPLATE("Push wiring works through typed block references",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  using ScopeOutput = std::conditional_t<std::is_same_v<T, f32>, push::f_32::sinks::ScopeF32Output,
                                         push::f_64::sinks::ScopeF64Output>;
  auto scope = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32(0);
    else
      return push::f_64::sinks::ScopeF64(0);
  }();
  auto constant = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::ConstF32(1, T{3});
    else
      return push::f_64::sources::ConstF64(1, T{3});
  }();
  core::function<ScopeOutput()>                  &sink = scope;
  core::function<void(push::DownstreamInput<T>)> &source = constant;
  auto                                            output = sink();
  source({.downstream = output.channels(2)});
  MockRuntime::start();
  if constexpr (std::is_same_v<T, f32>) {
    CHECK_EQ(MockRuntime::lastF32(0, 0), 3);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 3);
  } else {
    CHECK_EQ(MockRuntime::lastF64(0, 0), 3);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 3);
  }
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Scope rebuilds channel vectors when rebound",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  auto scope = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sinks::ScopeF32(0);
    else
      return push::f_64::sinks::ScopeF64(0);
  }();
  const auto output = scope();
  for (const u8 count : {u8{0}, u8{2}, u8{255}, u8{1}, u8{0}}) {
    CAPTURE(count);
    const auto channels = output.channels(count);
    REQUIRE_EQ(channels.size(), count);
    for (u32 i = 0; i < channels.size(); ++i) {
      const auto value = static_cast<T>(i + 1);
      (*channels[i])(value);
      if constexpr (std::is_same_v<T, f32>) {
        CHECK_EQ(MockRuntime::lastF32(0, static_cast<u8>(i)), value);
      } else {
        CHECK_EQ(MockRuntime::lastF64(0, static_cast<u8>(i)), value);
      }
    }
  }
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Aggregation supports channel vectors at channel count boundaries",
                   T,
                   f32,
                   f64) {
  constexpr auto Sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32;
    else
      return push::f_64::transformers::SumF64;
  }();
  for (const u8 count : {u8{0}, u8{255}}) {
    CAPTURE(count);
    MockRuntime::reset();
    auto scope = [] {
      if constexpr (std::is_same_v<T, f32>)
        return push::f_32::sinks::ScopeF32(0);
      else
        return push::f_64::sinks::ScopeF64(0);
    }();
    auto       sum = Sum(1, 10);
    const auto channels = sum({.downstream = scope().channels(1)}).channels(count);
    REQUIRE_EQ(channels.size(), count);
    for (auto *channel : channels) {
      (*channel)(T{1});
    }
    MockRuntime::start();
    MockRuntime::tick();
    if constexpr (std::is_same_v<T, f32>) {
      CHECK_EQ(MockRuntime::hasF32(0, 0), count != 0);
      if (count != 0) {
        CHECK_EQ(MockRuntime::lastF32(0, 0), static_cast<T>(count));
      }
    } else {
      CHECK_EQ(MockRuntime::hasF64(0, 0), count != 0);
      if (count != 0) {
        CHECK_EQ(MockRuntime::lastF64(0, 0), static_cast<T>(count));
      }
    }
    MockRuntime::close();
  }
}

TEST_CASE("GPIO input preserves disconnected pin positions and fanout") {
  MockRuntime::reset();
  auto scope = push::f_32::sinks::ScopeF32(0);
  auto gpio = push::f_32::sources::GpioInF32(1, 7, core::array<u8>{{1, 3}});
  auto output = scope();
  gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{{}, output.channels(2)}}});
  MockRuntime::emitGpio(7, 1, true);
  CHECK_FALSE(MockRuntime::hasF32(0, 0));
  CHECK_FALSE(MockRuntime::hasF32(0, 1));
  MockRuntime::emitGpio(7, 3, true);
  CHECK_EQ(MockRuntime::lastF32(0, 0), 1);
  CHECK_EQ(MockRuntime::lastF32(0, 1), 1);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("A block returns independent vectorized and scalar output fields",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  auto                                  scopes = DualScope<DualScopeOutput<T>>(10);
  core::function<DualScopeOutput<T>()> &block = scopes;
  auto [channels, single] = block();

  auto channelList = channels(2);
  REQUIRE_EQ(channelList.size(), 2);
  REQUIRE(single != nullptr);
  (*channelList[0])(T{3});
  (*channelList[1])(T{5});
  (*single)(T{7});
  if constexpr (std::is_same_v<T, f32>) {
    CHECK_EQ(MockRuntime::lastF32(10, 0), 3);
    CHECK_EQ(MockRuntime::lastF32(10, 1), 5);
    CHECK_EQ(MockRuntime::lastF32(11, 0), 7);
  } else {
    CHECK_EQ(MockRuntime::lastF64(10, 0), 3);
    CHECK_EQ(MockRuntime::lastF64(10, 1), 5);
    CHECK_EQ(MockRuntime::lastF64(11, 0), 7);
  }
  MockRuntime::close();
}

TEST_CASE("Copies of a wired block share state and retain consumer addresses") {
  MockRuntime::reset();
  auto firstScope = push::f_32::sinks::ScopeF32(0);
  auto secondScope = push::f_32::sinks::ScopeF32(1);
  auto cosine = push::f_32::transformers::CosF32(2);
  auto input = cosine({.downstream = firstScope().channels(1)}).consumer;
  auto copy = cosine;
  copy({.downstream = secondScope().channels(1)});
  cosine = {};
  auto moved = std::move(copy);
  (*input)(0.f);
  CHECK_FALSE(MockRuntime::hasF32(0, 0));
  CHECK_EQ(MockRuntime::lastF32(1, 0), 1.f);
  MockRuntime::close();
}

TEST_CASE("Copies retain timer and GPIO state after original callables are released") {
  MockRuntime::reset();
  auto scope = push::f_32::sinks::ScopeF32(0);
  auto channels = scope().channels(2);
  auto generator = push::f_32::sources::CosGenF32(1, 25);
  auto gpio = push::f_32::sources::GpioInF32(2, 7, core::array<u8>(1, u8{3}));
  generator({.downstream = std::array{channels[0]}});
  gpio(
      {.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{std::array{channels[1]}}}});
  auto generatorOwner = generator;
  auto gpioOwner = gpio;
  generator = {};
  gpio = {};
  MockRuntime::start();
  MockRuntime::tick();
  MockRuntime::emitGpio(7, 3, true);
  CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  CHECK_EQ(MockRuntime::lastF32(0, 1), 1.f);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("Consumers bind capturing lambdas directly and accept null fanout entries",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       received = 0;
  u32                     calls = 0;
  core::function<void(T)> receive = [&received, &calls](const T value) {
    received = value;
    ++calls;
  };
  auto source = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::ConstF32(0, T{3});
    else
      return push::f_64::sources::ConstF64(0, T{3});
  }();
  source({.downstream = std::array<core::function<void(T)> *, 2>{nullptr, &receive}});
  MockRuntime::start();
  CHECK_EQ(received, T{3});
  CHECK_EQ(calls, 1);
  MockRuntime::close();
}

TEST_CASE("A retained vectorized scope output owns consumers after the factory is released") {
  MockRuntime::reset();
  auto output = push::f_32::sinks::ScopeF32(0)();
  auto consumers = output.channels(2);
  auto copy = output.channels;
  output = {};
  (*consumers[0])(3.f);
  (*consumers[1])(5.f);
  CHECK_EQ(MockRuntime::lastF32(0, 0), 3.f);
  CHECK_EQ(MockRuntime::lastF32(0, 1), 5.f);
  MockRuntime::close();
}

TEST_CASE("Retained aggregate outputs preserve callbacks after the factory is released") {
  MockRuntime::reset();
  f32                       received = 0;
  core::function<void(f32)> receive = [&received](const f32 value) { received = value; };
  auto output = push::f_32::transformers::SumF32(0, 25)({.downstream = std::array{&receive}});
  auto consumers = output.channels(2);
  (*consumers[0])(3.f);
  (*consumers[1])(4.f);
  MockRuntime::start();
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  MockRuntime::tick();
  CHECK_EQ(received, 7.f);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
}

TEST_CASE_TEMPLATE(
    "Span inputs copy wiring from arrays and vectors before their storage is released",
    T,
    f32,
    f64) {
  MockRuntime::reset();
  constexpr auto Constant = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::ConstF32;
    else
      return push::f_64::sources::ConstF64;
  }();
  auto sources = std::array{Constant(0, T{2}), Constant(1, T{2}), Constant(2, T{2})};
  T    received = 0;
  u32  calls = 0;
  core::function<void(T)> receive = [&received, &calls](const T value) {
    received += value;
    ++calls;
  };
  {
    std::array               array{&receive};
    std::vector              vector{&receive};
    core::function<void(T)> *raw[]{&receive};
    sources[0]({.downstream = array});
    sources[1]({.downstream = vector});
    sources[2]({.downstream = raw});
    array[0] = nullptr;
    vector[0] = nullptr;
    raw[0] = nullptr;
  }
  MockRuntime::start();
  CHECK_EQ(received, T{6});
  CHECK_EQ(calls, 3);
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Periodic sources stay idle until start and honor configured intervals",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       sineValue = -1;
  T                       sineCopy = -1;
  T                       replaced = -1;
  T                       pulseValue = -1;
  T                       randomValue = -1;
  T                       sumValue = -1;
  u32                     sineCalls = 0;
  u32                     copyCalls = 0;
  u32                     replacedCalls = 0;
  core::function<void(T)> sineSink = [&](const T value) {
    sineValue = value;
    ++sineCalls;
  };
  core::function<void(T)> sineOther = [&](const T value) {
    sineCopy = value;
    ++copyCalls;
  };
  core::function<void(T)> nextSink = [&](const T value) {
    replaced = value;
    ++replacedCalls;
  };
  core::function<void(T)> pulseSink = [&](const T value) { pulseValue = value; };
  core::function<void(T)> randomSink = [&](const T value) { randomValue = value; };
  core::function<void(T)> sumSink = [&](const T value) { sumValue = value; };
  constexpr auto          Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::SinGenF32;
    else
      return push::f_64::sources::SinGenF64;
  }();
  constexpr auto Pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32;
    else
      return push::f_64::sources::PulseGenF64;
  }();
  constexpr auto Random = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::RandGenF32;
    else
      return push::f_64::sources::RandGenF64;
  }();
  constexpr auto Sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32;
    else
      return push::f_64::transformers::SumF64;
  }();

  // Wiring happens at 500 ms. The time base must be the later start instant.
  MockRuntime::setNow(500);
  auto sine = Sine(1, 25, T{1}, T{1}, T{0});
  auto pulse = Pulse(2, T{0.5}, T{1}, T{1}, T{0});
  auto random = Random(3, 40, T{2});
  auto sum = Sum(4, 15);
  sine({.downstream = std::array<core::function<void(T)> *, 3>{nullptr, &sineSink, &sineOther}});
  pulse({.downstream = std::array{&pulseSink}});
  random({.downstream = std::array{&randomSink}});
  auto terms = sum({.downstream = std::array{&sumSink}}).channels(1);
  (*terms[0])(T{4});

  MockRuntime::setRandom(0.25f);
  MockRuntime::tick();
  CHECK_EQ(sineCalls, 0);
  CHECK_EQ(copyCalls, 0);
  CHECK_EQ(pulseValue, T{-1});
  CHECK_EQ(randomValue, T{-1});
  CHECK_EQ(sumValue, T{-1});
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);

  MockRuntime::setNow(0);
  MockRuntime::start();
  CHECK_EQ(sineCalls, 0);
  CHECK_EQ(MockRuntime::activeIntervalCount(), 4);
  CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  CHECK_EQ(MockRuntime::intervalPeriodAt(1), 1);
  CHECK_EQ(MockRuntime::intervalPeriodAt(2), 40);
  CHECK_EQ(MockRuntime::intervalPeriodAt(3), 15);

  MockRuntime::setNow(250);
  MockRuntime::tick();
  CHECK_EQ(sineCalls, 1);
  CHECK_EQ(copyCalls, 1);
  CHECK(sineValue == doctest::Approx(1).epsilon(1e-4));
  CHECK(sineCopy == doctest::Approx(1).epsilon(1e-4));
  CHECK_EQ(pulseValue, T{1});
  CHECK_EQ(randomValue, T{0.5});
  CHECK_EQ(sumValue, T{4});

  sine({.downstream = std::array{&nextSink}});
  MockRuntime::tick();
  CHECK_EQ(sineCalls, 1);
  CHECK_EQ(copyCalls, 1);
  CHECK_EQ(replacedCalls, 1);
  CHECK(replaced == doctest::Approx(1).epsilon(1e-4));
  CHECK_EQ(MockRuntime::activeIntervalCount(), 4);
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Sum folds forward from the first term",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  T                       received = 0;
  core::function<void(T)> sink = [&](const T value) { received = value; };
  constexpr auto          Sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32;
    else
      return push::f_64::transformers::SumF64;
  }();
  const auto huge = static_cast<T>(1e20);
  auto       sum = Sum(1, 10);
  auto       terms = sum({.downstream = std::array{&sink}}).channels(3);
  (*terms[0])(huge);
  (*terms[1])(-huge);
  (*terms[2])(T{1});
  MockRuntime::start();
  CHECK_EQ(received, T{0});
  MockRuntime::tick();
  CHECK_EQ(received, T{1});
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Product publishes after a missing factor becomes finite",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  u32                     calls = 0;
  u32                     otherCalls = 0;
  T                       received = 0;
  T                       other = 0;
  core::function<void(T)> sink = [&](const T value) {
    received = value;
    ++calls;
  };
  core::function<void(T)> otherSink = [&](const T value) {
    other = value;
    ++otherCalls;
  };
  constexpr auto Product = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::ProductF32;
    else
      return push::f_64::transformers::ProductF64;
  }();
  auto factors = Product(1, 10)(
      {.downstream = std::array<core::function<void(T)> *, 3>{&sink, nullptr, &otherSink}});
  auto inputs = factors.channels(2);

  (*inputs[0])(T{5});
  MockRuntime::start();
  MockRuntime::tick();
  CHECK_EQ(calls, 0);
  CHECK_EQ(otherCalls, 0);

  (*inputs[1])(T{4});
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(otherCalls, 1);
  CHECK_EQ(received, T{20});
  CHECK_EQ(other, T{20});

  (*inputs[0])(std::numeric_limits<T>::infinity());
  MockRuntime::tick();
  CHECK_EQ(calls, 1);
  CHECK_EQ(otherCalls, 1);

  const auto tiny = std::numeric_limits<T>::denorm_min();
  REQUIRE(tiny != T{0});
  REQUIRE(core::isfinite(tiny));
  (*inputs[0])(tiny);
  (*inputs[1])(T{2});
  MockRuntime::tick();
  CHECK_EQ(calls, 2);
  CHECK_EQ(otherCalls, 2);
  CHECK_EQ(received, tiny * T{2});
  CHECK_EQ(other, tiny * T{2});

  MockRuntime::close();
  (*inputs[0])(T{3});
  (*inputs[1])(T{3});
  MockRuntime::tick();
  CHECK_EQ(calls, 2);
  CHECK_EQ(otherCalls, 2);
}
