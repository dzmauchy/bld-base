#include <array>
#include <doctest/doctest.h>

#include <core/types.hpp>
#include <vector>

namespace {

struct ChannelTarget {
  i32 value = 42;
  u8  lastCount = 0;
  u32 calls = 0;

  std::vector<i32 *> pointers;

  VectorizedInput<i32> channels(const u8 count) {
    lastCount = count;
    ++calls;
    pointers.assign(count, &value);
    return pointers;
  }
};

VectorizedInput<i32> nullChannels(const u8 count) {
  static const std::array<i32 *, 255> pointers{};
  return VectorizedInput<i32>{pointers}.first(count);
}

struct ChannelCallable {
  ChannelTarget       *target;
  VectorizedInput<i32> operator()(const u8 count) const { return target->channels(count); }
};

template <typename T, typename Source>
concept CanBindOutput =
    requires(Source &&source) { VectorizedOutput<T>(static_cast<Source &&>(source)); };

static_assert(CanBindOutput<i32,
                            ChannelCallable &>);
static_assert(!CanBindOutput<f32,
                             ChannelCallable &>);
static_assert(CanBindOutput<i32,
                            ChannelCallable>);
static_assert(std::is_same_v<VectorizedInput<i32>,
                             core::span<i32 *const>>);
static_assert(std::is_same_v<VectorizedOutput<i32>,
                             core::function<core::span<i32 *const>(u8)>>);

} // namespace

TEST_CASE("Vectorized output is empty when unbound") {
  const VectorizedOutput<i32> output;
  CHECK_FALSE(static_cast<bool>(output));

  const VectorizedOutput<i32> nullFunction = nullptr;
  CHECK_FALSE(static_cast<bool>(nullFunction));
}

TEST_CASE("Vectorized output forwards channel counts to a member function") {
  ChannelTarget               target;
  const VectorizedOutput<i32> output = [&target](const u8 count) { return target.channels(count); };
  CHECK(static_cast<bool>(output));

  for (const u8 count : {u8{0}, u8{1}, u8{2}, u8{255}}) {
    CAPTURE(count);
    const auto result = output(count);
    CHECK_EQ(target.lastCount, count);
    CHECK_EQ(result.size(), count);
    for (auto *channel : result) {
      CHECK_EQ(channel, &target.value);
    }
  }
  CHECK_EQ(target.calls, 4);
}

TEST_CASE("Vectorized output forwards channel counts to a plain function") {
  const VectorizedOutput<i32> output = nullChannels;
  CHECK(static_cast<bool>(output));

  for (const u8 count : {u8{0}, u8{1}, u8{2}, u8{255}}) {
    CAPTURE(count);
    const auto result = output(count);
    CHECK_EQ(result.size(), count);
    for (auto *channel : result) {
      CHECK(channel == nullptr);
    }
  }
}

TEST_CASE("Vectorized output owns a callable constructed from a temporary") {
  ChannelTarget               target;
  const VectorizedOutput<i32> output = ChannelCallable{&target};
  CHECK(static_cast<bool>(output));
  const auto result = output(3);
  CHECK_EQ(target.calls, 1);
  CHECK_EQ(target.lastCount, 3);
  REQUIRE_EQ(result.size(), 3);
  CHECK_EQ(result[0], &target.value);
}

TEST_CASE("Vectorized output copies continue to reference the same target") {
  ChannelTarget         target;
  VectorizedOutput<i32> original = [&target](const u8 count) { return target.channels(count); };
  const auto            copy = original;
  VectorizedOutput<i32> assigned;
  assigned = original;
  original = {};

  const auto first = copy(1);
  REQUIRE_EQ(first.size(), 1);
  CHECK_EQ(first[0], &target.value);
  auto      *firstConsumer = first[0];
  const auto second = assigned(2);
  REQUIRE_EQ(second.size(), 2);
  CHECK_EQ(target.calls, 2);
  CHECK_EQ(second[0], &target.value);
  target.value = 7;
  CHECK_EQ(*firstConsumer, 7);
  CHECK_EQ(*second[0], 7);
}

TEST_CASE("Vectorized output owns captured values and copies their state") {
  VectorizedOutput<i32> output;
  {
    std::vector<i32> values{7};
    output = [values,
              pointers = std::vector<i32 *>{}](const u8 count) mutable -> VectorizedInput<i32> {
      pointers.assign(count, &values[0]);
      return pointers;
    };
  }
  const auto copy = output;
  const auto originalValues = output(1);
  const auto copiedValues = copy(1);
  REQUIRE_EQ(originalValues.size(), 1);
  REQUIRE_EQ(copiedValues.size(), 1);
  CHECK_EQ(*originalValues[0], 7);
  CHECK_EQ(*copiedValues[0], 7);
  CHECK(originalValues[0] != copiedValues[0]);
  *originalValues[0] = 9;
  CHECK_EQ(*copiedValues[0], 7);
}
