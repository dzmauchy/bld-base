#include <core/lib.hpp>
#include <cstdint>
#include <doctest/doctest.h>
#include <memory>
#include <type_traits>
#include <utility>

namespace {
int twice(int value) { return value * 2; }

struct Tracked {
  inline static int alive = 0;
  int               value;
  explicit Tracked(int value = 0) : value(value) { ++alive; }
  Tracked(const Tracked &other) : Tracked(other.value) {}
  Tracked(Tracked &&other) noexcept : Tracked(other.value) { other.value = -1; }
  ~Tracked() { --alive; }
};

struct ThrowingCopy {
  inline static int alive = 0;
  inline static int copiesLeft = 100;
  int               value;
  explicit ThrowingCopy(int value = 0) : value(value) { ++alive; }
  ThrowingCopy(const ThrowingCopy &other) : value(other.value) {
    if (copiesLeft <= 0)
      throw 42;
    --copiesLeft;
    ++alive;
  }
  ~ThrowingCopy() { --alive; }
};

struct alignas(128) Aligned {
  int value;
};

static_assert(std::is_constructible_v<core::function<int(int)>,
                                      decltype(twice) &>);
static_assert(std::is_constructible_v<core::function<void(int)>,
                                      decltype(twice) &>);
static_assert(!std::is_constructible_v<core::function<int()>,
                                       decltype(twice) &>);
static_assert(!std::is_constructible_v<core::function<int(int)>,
                                       int>);
static_assert(!std::is_constructible_v<core::function<int()>,
                                       decltype([] { return; })>);
static_assert(!std::is_constructible_v<core::function<void()>,
                                       decltype([value = std::make_unique<int>()] {})>);
static_assert(!std::is_copy_constructible_v<core::array<std::unique_ptr<int>>>);
static_assert(!std::is_copy_assignable_v<core::array<std::unique_ptr<int>>>);
} // namespace

TEST_CASE("Core function owns captures and copies mutable state independently") {
  core::function<int(int)> callable;
  CHECK_FALSE(callable);
  callable = [sum = 3](int value) mutable { return sum += value; };
  CHECK_EQ(callable(2), 5);
  const auto copy = callable;
  CHECK_EQ(copy(1), 6);
  CHECK_EQ(callable(2), 7);
  auto &callableAlias = callable;
  callable = callableAlias;
  CHECK_EQ(callable(1), 8);
  auto moved = std::move(callable);
  CHECK_FALSE(callable);
  CHECK_EQ(moved(2), 10);
  auto &movedAlias = moved;
  moved = std::move(movedAlias);
  CHECK_EQ(moved(1), 11);
  moved = nullptr;
  CHECK_FALSE(moved);
  CHECK_EQ(copy(1), 7);
}

TEST_CASE("Core function supports free functions, references, void and move-only arguments") {
  core::function<int(int)> callable = twice;
  CHECK_EQ(callable(3), 6);
  int (*nullFunction)(int) = nullptr;
  callable = nullFunction;
  CHECK_FALSE(callable);
  callable = twice;
  core::function<void(int)> discard = twice;
  discard(3);
  int                          value = 1;
  core::function<int &(int &)> reference = [](int &v) -> int & { return v; };
  reference(value) = 4;
  CHECK_EQ(value, 4);
  core::function<int(std::unique_ptr<int>)> consume = [](auto v) { return *v; };
  CHECK_EQ(consume(std::make_unique<int>(8)), 8);
  core::function<int()> converted = [] { return short{9}; };
  CHECK_EQ(converted(), 9);
}

TEST_CASE("Core function releases captured objects on replacement and destruction") {
  REQUIRE_EQ(Tracked::alive, 0);
  {
    core::function<int()> callable = [value = Tracked{4}] { return value.value; };
    CHECK_EQ(Tracked::alive, 1);
    auto copy = callable;
    CHECK_EQ(Tracked::alive, 2);
    callable = [] { return 7; };
    CHECK_EQ(Tracked::alive, 1);
    CHECK_EQ(copy(), 4);
    copy.reset();
    CHECK_EQ(Tracked::alive, 0);
    core::function<int()> aligned = [value = Aligned{12}] { return value.value; };
    CHECK_EQ(aligned(), 12);
  }
  CHECK_EQ(Tracked::alive, 0);
}

TEST_CASE("Core array has a size set at construction and copies raw buffers") {
  core::array<int> empty;
  CHECK(empty.empty());
  CHECK_EQ(empty.begin(), empty.end());
  core::array<int> zeroCount(0);
  CHECK(zeroCount.empty());
  core::array<int> emptyRange(nullptr, nullptr);
  CHECK(emptyRange.empty());
  core::array<int> zeros(3);
  REQUIRE_EQ(zeros.size(), 3);
  CHECK_EQ(zeros[2], 0);
  core::array<int> repeated(3, 7);
  CHECK_EQ(repeated.back(), 7);
  repeated.fill(4);
  for (const auto value : repeated)
    CHECK_EQ(value, 4);
  core::array<int> literal{{1, 2, 3}};
  CHECK_EQ(literal[1], 2);
  const int   raw[]{4, 5};
  core::array copied(raw);
  CHECK_EQ(copied.front(), 4);
  const auto copy = copied;
  copied[0] = 9;
  CHECK_EQ(copy[0], 4);
  auto *storage = copied.data();
  auto &copiedAlias = copied;
  copied = copiedAlias;
  CHECK_EQ(copied.data(), storage);
  auto moved = std::move(copied);
  CHECK(copied.empty());
  CHECK_EQ(moved.data(), storage);
  CHECK_EQ(moved.front(), 9);
  auto &movedAlias = moved;
  moved = std::move(movedAlias);
  CHECK_EQ(moved.data(), storage);
  moved = copy;
  CHECK_EQ(moved.back(), 5);
  CHECK_NE(moved.data(), copy.data());
  core::array<int> range(copy.begin(), copy.end());
  CHECK_EQ(range.back(), 5);
  CHECK_EQ(range.at(1), 5);
  const core::array<int> &constant = range;
  CHECK_EQ(constant.at(0), 4);
}

TEST_CASE("Core array destroys every element when its fixed storage is released") {
  REQUIRE_EQ(Tracked::alive, 0);
  {
    core::array<Tracked> values(3);
    CHECK_EQ(Tracked::alive, 3);
    values[0].value = 7;
    auto copied = values;
    CHECK_EQ(Tracked::alive, 6);
    copied[0].value = 4;
    CHECK_EQ(values[0].value, 7);
    auto moved = std::move(copied);
    CHECK(copied.empty());
    CHECK_EQ(Tracked::alive, 6);
    values = std::move(moved);
    CHECK(moved.empty());
    CHECK_EQ(Tracked::alive, 3);
    CHECK_EQ(values.front().value, 4);
    core::array<Tracked> empty;
    CHECK_EQ(Tracked::alive, 3);
  }
  CHECK_EQ(Tracked::alive, 0);
}

TEST_CASE("Core array supports move-only, aligned and callable elements") {
  core::array<std::unique_ptr<int>> pointers(2);
  pointers[0] = std::make_unique<int>(4);
  pointers[1] = std::make_unique<int>(7);
  auto moved = std::move(pointers);
  CHECK(pointers.empty());
  CHECK_EQ(*moved[0], 4);
  core::array<Aligned> aligned(5);
  aligned[0].value = 12;
  CHECK_EQ(reinterpret_cast<std::uintptr_t>(aligned.data()) % alignof(Aligned), 0);
  CHECK_EQ(aligned[0].value, 12);
  core::array<core::function<int(int)>> callbacks(31);
  callbacks[0] = [value = 1](int increment) mutable { return value += increment; };
  for (std::size_t i = 1; i < callbacks.size(); ++i)
    callbacks[i] = twice;
  auto copied = callbacks;
  CHECK_EQ(callbacks[0](2), 3);
  CHECK_EQ(copied[0](3), 4);
  CHECK_EQ(copied.back()(3), 6);
}

TEST_CASE("Core array cleans up partial construction and failed copies") {
  REQUIRE_EQ(ThrowingCopy::alive, 0);
  {
    core::array<ThrowingCopy> values(2);
    values[0].value = 3;
    values[1].value = 5;
    ThrowingCopy::copiesLeft = 1;
    CHECK_THROWS_AS(core::array<ThrowingCopy>(3, values[0]), int);
    CHECK_EQ(ThrowingCopy::alive, 2);
    ThrowingCopy::copiesLeft = 1;
    CHECK_THROWS_AS((core::array<ThrowingCopy>(values)), int);
    CHECK_EQ(ThrowingCopy::alive, 2);
    core::array<ThrowingCopy> destination(1);
    destination[0].value = 9;
    auto *storage = destination.data();
    ThrowingCopy::copiesLeft = 1;
    CHECK_THROWS_AS(destination = values, int);
    CHECK_EQ(ThrowingCopy::alive, 3);
    CHECK_EQ(destination.data(), storage);
    CHECK_EQ(destination.size(), 1);
    CHECK_EQ(destination.front().value, 9);
    ThrowingCopy::copiesLeft = 100;
  }
  CHECK_EQ(ThrowingCopy::alive, 0);
}

TEST_CASE("Core span borrows storage and can expose a shorter prefix") {
  core::span<int> empty;
  CHECK(empty.empty());
  CHECK_EQ(empty.data(), nullptr);
  CHECK_EQ(empty.begin(), empty.end());

  int             values[]{1, 2, 3, 4};
  core::span<int> full(values);
  REQUIRE_EQ(full.size(), 4);
  CHECK_EQ(full[3], 4);
  auto prefix = full.first(2);
  REQUIRE_EQ(prefix.size(), 2);
  CHECK_EQ(prefix[1], 2);
  prefix[1] = 9;
  CHECK_EQ(values[1], 9);
  CHECK(full.first(0).empty());
  CHECK_EQ(full.first(full.size()).size(), 4);
  CHECK_EQ(full.first(full.size())[3], 4);

  core::array<int> owned{{5, 6, 7}};
  core::span<int>  borrowed(owned);
  REQUIRE_EQ(borrowed.size(), 3);
  core::array<int> copied(borrowed.first(2));
  borrowed[0] = 8;
  CHECK_EQ(owned[0], 8);
  REQUIRE_EQ(copied.size(), 2);
  CHECK_EQ(copied[0], 5);
  CHECK_EQ(copied[1], 6);
}

TEST_CASE("Core shared pointer keeps one object alive across copies and releases it once") {
  REQUIRE_EQ(Tracked::alive, 0);
  {
    core::shared_ptr<Tracked> empty;
    CHECK_FALSE(empty);
    CHECK_EQ(empty.get(), nullptr);
    auto value = core::make_shared<Tracked>(4);
    CHECK(static_cast<bool>(value));
    CHECK_EQ(value->value, 4);
    CHECK_EQ((*value).value, 4);
    CHECK_EQ(Tracked::alive, 1);

    auto copy = value;
    CHECK_EQ(copy.get(), value.get());
    copy->value = 9;
    CHECK_EQ(value->value, 9);
    CHECK_EQ(Tracked::alive, 1);

    auto moved = std::move(value);
    CHECK_FALSE(value);
    CHECK_EQ(moved->value, 9);
    auto swapped = core::make_shared<Tracked>(2);
    moved.swap(swapped);
    CHECK_EQ(moved->value, 2);
    CHECK_EQ(swapped->value, 9);
    CHECK_EQ(Tracked::alive, 2);

    auto &alias = swapped;
    swapped = alias;
    CHECK_EQ(swapped.get(), alias.get());
    CHECK_EQ(swapped->value, 9);
    swapped = std::move(alias);
    CHECK(swapped);
    CHECK_EQ(swapped->value, 9);
    CHECK_EQ(Tracked::alive, 2);

    core::shared_ptr<Tracked> replacement = core::make_shared<Tracked>(1);
    CHECK_EQ(Tracked::alive, 3);
    replacement = std::move(swapped);
    CHECK_FALSE(swapped);
    CHECK_EQ(replacement->value, 9);
    CHECK_EQ(Tracked::alive, 2);
    replacement = empty;
    CHECK_FALSE(replacement);
    CHECK_EQ(Tracked::alive, 2);
    copy.reset();
    CHECK_EQ(Tracked::alive, 1);
    moved.reset();
    CHECK_EQ(Tracked::alive, 0);

    auto again = core::make_shared<Tracked>(3);
    CHECK_EQ(Tracked::alive, 1);
    again = core::shared_ptr<Tracked>{};
    CHECK_EQ(Tracked::alive, 0);
  }
  CHECK_EQ(Tracked::alive, 0);
}
