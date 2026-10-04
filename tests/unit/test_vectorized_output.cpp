#include <doctest/doctest.h>

#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <type_traits>

#include "../mock_runtime.hpp"

namespace {

u32 slotA = 1;
u32 slotB = 2;
u32 slotC = 3;

auto slotsFor(const u8 count) -> Array<u32 *> {
  u32 *slots[] = {&slotA, &slotB, &slotC};
  auto result = Array<u32 *>{};
  for (u8 i = 0; i < count && i < 3; ++i) {
    result.push_back(slots[i]);
  }
  return result;
}

struct Binder {
  auto open(const u8 count) -> Array<u32 *> {
    seen = count;
    ++calls;
    auto result = Array<u32 *>{};
    for (u8 i = 0; i < count; ++i) {
      result.push_back(&token);
    }
    return result;
  }

  u8   seen{255};
  int  calls{0};
  u32  token{42};
};

template <typename T> struct ChannelProbe;

template <> struct ChannelProbe<f32> {
  using Scope = push::f_32::sinks::ScopeF32;
  using Product = push::f_32::transformers::ProductF32;

  static auto has(const u32 blockId, const u8 channel) -> bool { return MockRuntime::hasF32(blockId, channel); }
  static auto last(const u32 blockId, const u8 channel) -> f32 { return MockRuntime::lastF32(blockId, channel); }
};

template <> struct ChannelProbe<f64> {
  using Scope = push::f_64::sinks::ScopeF64;
  using Product = push::f_64::transformers::ProductF64;

  static auto has(const u32 blockId, const u8 channel) -> bool { return MockRuntime::hasF64(blockId, channel); }
  static auto last(const u32 blockId, const u8 channel) -> f64 { return MockRuntime::lastF64(blockId, channel); }
};

}  // namespace

TEST_CASE("A default vectorized output invokes nothing") {
  const auto output = VectorizedOutput<u32>{};
  CHECK_FALSE(static_cast<bool>(output));
  CHECK_EQ(output(0).size(), 0);
  CHECK_EQ(output(4).size(), 0);

  const auto copy = output;
  CHECK_FALSE(static_cast<bool>(copy));
  CHECK(copy(2).empty());
}

TEST_CASE("A function vectorized output forwards the requested count") {
  const auto output = VectorizedOutput<u32>{slotsFor};
  CHECK(static_cast<bool>(output));

  const auto none = output(0);
  CHECK(none.empty());

  const auto two = output(2);
  REQUIRE_EQ(two.size(), 2);
  CHECK_EQ(two[0], &slotA);
  CHECK_EQ(two[1], &slotB);

  const auto copy = output;
  const auto three = copy(3);
  REQUIRE_EQ(three.size(), 3);
  CHECK_EQ(three[2], &slotC);
  CHECK_EQ(*three[0], 1);
}

TEST_CASE("A method vectorized output stays bound to its target") {
  auto binder = Binder{};
  const auto output = VectorizedOutput<u32>::from<&Binder::open>(&binder);
  CHECK(static_cast<bool>(output));

  const auto none = output(0);
  CHECK(none.empty());
  CHECK_EQ(binder.calls, 1);
  CHECK_EQ(binder.seen, 0);

  const auto copy = output;
  const auto bound = copy(2);
  REQUIRE_EQ(bound.size(), 2);
  CHECK_EQ(bound[0], &binder.token);
  CHECK_EQ(bound[1], &binder.token);
  CHECK_EQ(binder.calls, 2);
  CHECK_EQ(binder.seen, 2);
  CHECK_EQ(*bound[0], 42);
}

TEST_CASE_TEMPLATE("Repeated scope channel requests restart indexes at zero", T, f32, f64) {
  using Probe = ChannelProbe<T>;
  MockRuntime::reset();
  auto scope = typename Probe::Scope(0);
  const auto output = scope.apply();
  CHECK(static_cast<bool>(output.channels));

  const auto first = output.channels(1);
  REQUIRE_EQ(first.size(), 1);
  (*first[0])(T{3});
  CHECK_EQ(Probe::last(0, 0), T{3});
  CHECK_FALSE(Probe::has(0, 1));

  const auto copy = output;
  const auto none = copy.channels(0);
  CHECK(none.empty());
  CHECK_EQ(Probe::last(0, 0), T{3});

  const auto rebound = output.channels(2);
  REQUIRE_EQ(rebound.size(), 2);
  REQUIRE(rebound[0] != nullptr);
  REQUIRE(rebound[1] != nullptr);
  CHECK(rebound[0] != rebound[1]);
  (*rebound[0])(T{8});
  (*rebound[1])(T{9});
  CHECK_EQ(Probe::last(0, 0), T{8});
  CHECK_EQ(Probe::last(0, 1), T{9});
  CHECK_FALSE(Probe::has(0, 2));
}

TEST_CASE_TEMPLATE("Rebinding aggregate channels drops stale factors", T, f32, f64) {
  using Probe = ChannelProbe<T>;
  MockRuntime::reset();
  auto scope = typename Probe::Scope(0);
  auto product = typename Probe::Product(1);
  const auto sinks = scope.apply().channels(1);
  const auto output = product.apply({.downstream = sinks});
  const auto factors = output.channels(2);
  REQUIRE_EQ(factors.size(), 2);
  (*factors[0])(T{3});
  (*factors[1])(T{4});

  MockRuntime::start();
  MockRuntime::tick();
  CHECK_EQ(Probe::last(0, 0), T{12});

  const auto none = output.channels(0);
  CHECK(none.empty());
  MockRuntime::tick();
  CHECK_EQ(Probe::last(0, 0), T{12});

  const auto rebound = output.channels(1);
  REQUIRE_EQ(rebound.size(), 1);
  MockRuntime::tick();
  CHECK_EQ(Probe::last(0, 0), T{12});

  (*rebound[0])(T{-5});
  MockRuntime::tick();
  CHECK_EQ(Probe::last(0, 0), T{-5});
}
