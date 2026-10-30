#include <array>
#include <doctest/doctest.h>

#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <concepts>
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

// The first sink records the in-flight sample. The second sink throws until accepted.
// The third sink must stay silent for that attempt.
template <typename T> struct ThrowingFanout {
  bool throwing = true;
  T    firstValue{};
  T    secondValue{};
  T    thirdValue{T{-1}};
  u32  firstCalls = 0;
  u32  secondCalls = 0;
  u32  thirdCalls = 0;

  core::function<void(T)> first{[this](const T value) {
    firstValue = value;
    ++firstCalls;
  }};
  core::function<void(T)> second{[this](const T value) {
    secondValue = value;
    if (throwing)
      throw 1;
    ++secondCalls;
  }};
  core::function<void(T)> third{[this](const T value) {
    thirdValue = value;
    ++thirdCalls;
  }};

  ThrowingFanout() = default;
  ThrowingFanout(const ThrowingFanout &) = delete;
  ThrowingFanout &operator=(const ThrowingFanout &) = delete;

  std::array<core::function<void(T)> *, 3> sinks() { return {&first, &second, &third}; }

  void expectAborted(const char *label,
                     T            sample) const {
    INFO(label);
    CHECK_EQ(firstCalls, 1);
    CHECK_EQ(firstValue, sample);
    CHECK_EQ(secondCalls, 0);
    CHECK_EQ(secondValue, sample);
    CHECK_EQ(thirdCalls, 0);
    CHECK_EQ(thirdValue, T{-1});
  }

  void expectDelivered(const char *label,
                       T            sample) const {
    INFO(label);
    CHECK_EQ(firstCalls, 1);
    CHECK_EQ(secondCalls, 1);
    CHECK_EQ(thirdCalls, 1);
    CHECK_EQ(firstValue, sample);
    CHECK_EQ(secondValue, sample);
    CHECK_EQ(thirdValue, sample);
  }

  void acceptNext() {
    throwing = false;
    firstCalls = 0;
    secondCalls = 0;
    thirdCalls = 0;
    thirdValue = T{-1};
  }
};

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

TEST_CASE_TEMPLATE("A throwing sink stops later fan-out and the next sample reaches every sink",
                   T,
                   f32,
                   f64) {
  constexpr auto Constant = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::ConstF32;
    else
      return push::f_64::sources::ConstF64;
  }();
  constexpr auto Sine = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SinF32;
    else
      return push::f_64::transformers::SinF64;
  }();
  constexpr auto Sum = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::transformers::SumF32;
    else
      return push::f_64::transformers::SumF64;
  }();
  constexpr auto Pulse = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::PulseGenF32;
    else
      return push::f_64::sources::PulseGenF64;
  }();
  constexpr auto Gpio = [] {
    if constexpr (std::is_same_v<T, f32>)
      return push::f_32::sources::GpioInF32;
    else
      return push::f_64::sources::GpioInF64;
  }();

  {
    MockRuntime::reset();
    ThrowingFanout<T> fan;
    auto              constant = Constant(1, T{5});
    constant({.downstream = fan.sinks()});
    CHECK_THROWS_AS(MockRuntime::start(), int);
    fan.expectAborted("constant", T{5});
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
    fan.acceptNext();
    MockRuntime::start();
    fan.expectDelivered("constant retry", T{5});
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }

  {
    MockRuntime::reset();
    ThrowingFanout<T> fan;
    auto              transformer = Sine(2);
    auto              output = transformer({.downstream = fan.sinks()});
    CHECK_THROWS_AS((*output.consumer)(T{0}), int);
    fan.expectAborted("unary", T{0});
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
    fan.acceptNext();
    (*output.consumer)(T{0});
    fan.expectDelivered("unary retry", T{0});
  }

  {
    MockRuntime::reset();
    ThrowingFanout<T> fan;
    auto              sum = Sum(3, 15);
    auto              output = sum({.downstream = fan.sinks()});
    auto              channels = output.channels(2);
    (*channels[0])(T{3});
    (*channels[1])(T{4});
    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 15);
    CHECK_THROWS_AS(MockRuntime::tick(), int);
    fan.expectAborted("sum", T{7});
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 15);
    (*channels[1])(T{6});
    fan.acceptNext();
    MockRuntime::tick();
    fan.expectDelivered("sum retry", T{9});
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
    MockRuntime::tick();
    fan.expectDelivered("sum after close", T{9});
  }

  {
    MockRuntime::reset();
    ThrowingFanout<T> fan;
    auto              pulse = Pulse(4, T{1}, T{6}, T{0}, T{0});
    pulse({.downstream = fan.sinks()});
    MockRuntime::setNow(0);
    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);
    CHECK_THROWS_AS(MockRuntime::tick(), int);
    fan.expectAborted("pulse", T{6});
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    fan.acceptNext();
    MockRuntime::tick();
    fan.expectDelivered("pulse retry", T{6});
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
    MockRuntime::tick();
    fan.expectDelivered("pulse after close", T{6});
  }

  {
    MockRuntime::reset();
    ThrowingFanout<T>       fan;
    T                       otherValue = T{-1};
    u32                     otherCalls = 0;
    core::function<void(T)> other = [&](const T value) {
      otherValue = value;
      ++otherCalls;
    };
    auto gpio = Gpio(5, 6, core::array<u8>{{1, 4}});
    gpio({.pins = core::array<VectorizedInput<core::function<void(T)>>>{
              {fan.sinks(), std::array{&other}}}});
    CHECK_EQ(MockRuntime::activeGpioCount(), 2);
    CHECK_THROWS_AS(MockRuntime::emitGpio(6, 1, true), int);
    fan.expectAborted("gpio", T{1});
    CHECK_EQ(otherCalls, 0);
    CHECK_EQ(otherValue, T{-1});
    CHECK_EQ(MockRuntime::activeGpioCount(), 2);
    MockRuntime::emitGpio(6, 4, true);
    CHECK_EQ(otherCalls, 1);
    CHECK_EQ(otherValue, T{1});
    fan.expectAborted("gpio other pin", T{1});
    fan.acceptNext();
    MockRuntime::emitGpio(6, 1, false);
    fan.expectDelivered("gpio retry", T{0});
    CHECK_EQ(otherCalls, 1);
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeGpioCount(), 0);
    MockRuntime::emitGpio(6, 1, true);
    MockRuntime::emitGpio(6, 4, false);
    fan.expectDelivered("gpio after close", T{0});
    CHECK_EQ(otherCalls, 1);
    CHECK_EQ(otherValue, T{1});
  }
}
