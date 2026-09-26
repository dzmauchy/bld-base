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
union UnionPort {
  i32 value;
};

template <typename I, typename O>
concept ValidBlock = requires { typename Block<I, O>; };

static_assert(ValidBlock<Inputs, Outputs>);
static_assert(ValidBlock<Empty, void>);
static_assert(!ValidBlock<void, Outputs>);
static_assert(!ValidBlock<i32, Outputs>);
static_assert(!ValidBlock<Inputs*, Outputs>);
static_assert(!ValidBlock<Inputs, i32>);
static_assert(!ValidBlock<Inputs, Outputs*>);
static_assert(!ValidBlock<UnionPort, Outputs>);
static_assert(!ValidBlock<Inputs, UnionPort>);

class Arithmetic final : public Block<Inputs, Outputs> {
 public:
  using Block::Block;
  Outputs apply(Inputs input) override { return {input.left + input.right, input.left - input.right}; }
};

class Trigger final : public Block<Empty, void> {
 public:
  using Block::Block;
  void apply(Empty) override { ++calls; }
  u32 calls{0};
};

template <typename B, typename ExpectedOutput>
constexpr bool hasBlockContract =
    requires(B& block, typename B::Input input) {
      { block.apply(input) } -> std::same_as<typename B::Output>;
    } && std::derived_from<B, Block<typename B::Input, typename B::Output>> && std::same_as<typename B::Output, ExpectedOutput> &&
    (std::is_void_v<ExpectedOutput> || (std::is_class_v<ExpectedOutput> && std::is_aggregate_v<ExpectedOutput>));

static_assert(hasBlockContract<push::f32::sources::ConstF32<>, void>);
static_assert(hasBlockContract<push::f64::sources::ConstF64<>, void>);
static_assert(hasBlockContract<push::f32::sinks::ScopeF32<>, push::f32::sinks::ScopeF32Output>);
static_assert(hasBlockContract<push::f64::sinks::ScopeF64<>, push::f64::sinks::ScopeF64Output>);
static_assert(hasBlockContract<push::f32::transformers::CosF32<>, push::f32::transformers::CosF32Output>);
static_assert(hasBlockContract<push::f64::transformers::CosF64<>, push::f64::transformers::CosF64Output>);
static_assert(hasBlockContract<push::f32::transformers::SinF32<>, push::f32::transformers::SinF32Output>);
static_assert(hasBlockContract<push::f64::transformers::SinF64<>, push::f64::transformers::SinF64Output>);
static_assert(hasBlockContract<push::f32::transformers::SumF32<>, push::f32::transformers::SumF32Output>);
static_assert(hasBlockContract<push::f64::transformers::SumF64<>, push::f64::transformers::SumF64Output>);
static_assert(hasBlockContract<push::f32::transformers::ProductF32<>, push::f32::transformers::ProductF32Output>);
static_assert(hasBlockContract<push::f64::transformers::ProductF64<>, push::f64::transformers::ProductF64Output>);
static_assert(hasBlockContract<push::f32::sources::CosGenF32<>, void>);
static_assert(hasBlockContract<push::f64::sources::CosGenF64<>, void>);
static_assert(hasBlockContract<push::f32::sources::SinGenF32<>, void>);
static_assert(hasBlockContract<push::f64::sources::SinGenF64<>, void>);
static_assert(hasBlockContract<push::f32::sources::RandGenF32<>, void>);
static_assert(hasBlockContract<push::f64::sources::RandGenF64<>, void>);
static_assert(hasBlockContract<push::f32::sources::PulseGenF32<>, void>);
static_assert(hasBlockContract<push::f64::sources::PulseGenF64<>, void>);
static_assert(hasBlockContract<push::f32::sources::GpioInF32<>, void>);
static_assert(hasBlockContract<push::f64::sources::GpioInF64<>, void>);

/**
 * DualScopeOutput
 * @brief Two independent output ports, one vectorized and one scalar.
 * @image scope.svg
 */
template <typename T>
struct DualScopeOutput {
  /**
   * Value
   * @brief The numeric type accepted by both output ports.
   * @image type.svg
   */
  using Value = T;

  /**
   * Channels
   * @brief One vectorized output carrying the first scope's consumers.
   * @image scope.svg
   */
  Vectorized<Consumer<T>> channels{};

  /**
   * Single
   * @brief A separate output carrying the second scope's consumer.
   * @image scope.svg
   */
  Consumer<T>* single{nullptr};
};

/**
 * DualScope
 * @brief Exposes two scopes as independent named output ports.
 * @image scope.svg
 */
template <typename I, typename O>
class DualScope final : public Block<I, O> {
  using T = typename O::Value;
  using ScopeOutput = std::conditional_t<std::is_same_v<T, f32>, push::f32::sinks::ScopeF32Output, push::f64::sinks::ScopeF64Output>;

 public:
  explicit DualScope(u32 blockId) : Block<I, O>(blockId), first_(blockId), second_(blockId + 1) {}

  O apply(I input) override { return {.channels = first_.apply(input).channels, .single = second_.apply({.channelCount = 1}).channels[0]}; }

 private:
  push::Scope<I, ScopeOutput> first_;
  push::Scope<I, ScopeOutput> second_;
};

}  // namespace

TEST_CASE("Block dispatches structs with multiple ports") {
  Arithmetic arithmetic(42);
  Block<Inputs, Outputs>& block = arithmetic;
  const auto output = block.apply({.left = 7, .right = 3});
  CHECK_EQ(block.id(), 42);
  CHECK_EQ(output.sum, 10);
  CHECK_EQ(output.difference, 4);
}

TEST_CASE("Block supports an empty input struct and void output") {
  Trigger trigger(1);
  Block<Empty, void>& block = trigger;
  block.apply({});
  CHECK_EQ(trigger.calls, 1);
}

TEST_CASE_TEMPLATE("Push wiring works through typed block references", T, f32, f64) {
  MockRuntime::reset();
  using ScopeOutput = std::conditional_t<std::is_same_v<T, f32>, push::f32::sinks::ScopeF32Output, push::f64::sinks::ScopeF64Output>;
  push::Scope<push::ScopeInput, ScopeOutput> scope(0);
  push::Constant<push::DownstreamInput<T>, void> constant(1, T{3});
  Block<push::ScopeInput, ScopeOutput>& sink = scope;
  Block<push::DownstreamInput<T>, void>& source = constant;
  auto output = sink.apply({.channelCount = 2});
  source.apply({.downstream = output.channels});
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

TEST_CASE("GPIO input preserves disconnected pin positions and fanout") {
  MockRuntime::reset();
  push::f32::sinks::ScopeF32<> scope(0);
  push::f32::sources::GpioInF32<> gpio(1, 7, {1, 3});
  auto output = scope.apply({.channelCount = 2});
  gpio.apply({.pins = {{}, output.channels}});
  MockRuntime::emitGpio(7, 1, true);
  CHECK_FALSE(MockRuntime::hasF32(0, 0));
  CHECK_FALSE(MockRuntime::hasF32(0, 1));
  MockRuntime::emitGpio(7, 3, true);
  CHECK_EQ(MockRuntime::lastF32(0, 0), 1);
  CHECK_EQ(MockRuntime::lastF32(0, 1), 1);
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("A block returns independent vectorized and scalar output fields", T, f32, f64) {
  MockRuntime::reset();
  DualScope<push::ScopeInput, DualScopeOutput<T>> scopes(10);
  Block<push::ScopeInput, DualScopeOutput<T>>& block = scopes;
  auto [channels, single] = block.apply({.channelCount = 2});

  REQUIRE_EQ(channels.size(), 2);
  REQUIRE(single != nullptr);
  (*channels[0])(T{3});
  (*channels[1])(T{5});
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
