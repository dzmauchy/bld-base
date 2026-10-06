#include <doctest/doctest.h>

#include <core/array.hpp>
#include <vector>

namespace {

struct ChannelTarget {
  i32 value = 42;
  u8  lastCount = 0;
  u32 calls = 0;

  std::vector<i32 *> channels(const u8 count) {
    lastCount = count;
    ++calls;
    return std::vector<i32 *>(count, &value);
  }
};

std::vector<i32 *> nullChannels(const u8 count) { return std::vector<i32 *>(count, nullptr); }

struct ChannelCallable {
  ChannelTarget     *target;
  std::vector<i32 *> operator()(const u8 count) const { return target->channels(count); }
};

template <typename T, typename Source>
concept CanBindOutput = requires(Source &&source) { VectorizedOutput<T>(static_cast<Source &&>(source)); };

static_assert(CanBindOutput<i32,
                            ChannelCallable &>);
static_assert(!CanBindOutput<f32,
                             ChannelCallable &>);
static_assert(CanBindOutput<i32,
                            ChannelCallable>);

} // namespace

TEST_CASE("VectorizedOutput throws bad_function_call when unbound") {
  const VectorizedOutput<i32> output;
  CHECK_FALSE(static_cast<bool>(output));
  CHECK_THROWS_AS(output(0), std::bad_function_call);
  CHECK_THROWS_AS(output(255), std::bad_function_call);

  const VectorizedOutput<i32> nullFunction = nullptr;
  CHECK_FALSE(static_cast<bool>(nullFunction));
  CHECK_THROWS_AS(nullFunction(2), std::bad_function_call);
}

TEST_CASE("VectorizedOutput forwards channel counts to a member function") {
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

TEST_CASE("VectorizedOutput forwards channel counts to a plain function") {
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

TEST_CASE("VectorizedOutput owns a callable constructed from a temporary") {
  ChannelTarget               target;
  const VectorizedOutput<i32> output = ChannelCallable{&target};
  CHECK(static_cast<bool>(output));
  const auto result = output(3);
  CHECK_EQ(target.calls, 1);
  CHECK_EQ(target.lastCount, 3);
  REQUIRE_EQ(result.size(), 3);
  CHECK_EQ(result[0], &target.value);
}

TEST_CASE("VectorizedOutput copies continue to reference the same target") {
  ChannelTarget         target;
  VectorizedOutput<i32> original = [&target](const u8 count) { return target.channels(count); };
  const auto            copy = original;
  VectorizedOutput<i32> assigned;
  assigned = original;
  original = {};

  const auto first = copy(1);
  const auto second = assigned(2);
  REQUIRE_EQ(first.size(), 1);
  REQUIRE_EQ(second.size(), 2);
  CHECK_EQ(target.calls, 2);
  CHECK_EQ(first[0], &target.value);
  CHECK_EQ(second[0], &target.value);
  target.value = 7;
  CHECK_EQ(*first[0], 7);
  CHECK_EQ(*second[0], 7);
}

TEST_CASE("VectorizedOutput owns captured values and copies their state") {
  VectorizedOutput<i32> output;
  {
    std::vector<i32> values{7};
    output = [values](const u8 count) mutable { return std::vector<i32 *>(count, &values[0]); };
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
