#include <doctest/doctest.h>

#include <core/array.hpp>
#include <core/hal.hpp>
#include <core/math/trig.hpp>
#include <core/maybe.hpp>
#include <limits>
#include <type_traits>

namespace {

struct Probe {
  static int alive;

  int value;

  explicit Probe(const int value) : value(value) { ++alive; }
  Probe(const Probe &other) : value(other.value) { ++alive; }
  Probe(Probe &&other) noexcept : value(other.value) {
    other.value = -1;
    ++alive;
  }
  Probe &operator=(const Probe &other) {
    value = other.value;
    return *this;
  }
  Probe &operator=(Probe &&other) noexcept {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~Probe() { --alive; }
};

int Probe::alive = 0;

template <typename T> T signalingNan() {
  if constexpr (std::is_same_v<T, f32>) {
    const u32 bits = 0x7f800001u;
    f32       value;
    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
  } else {
    const u64 bits = 0x7ff0000000000001ull;
    f64       value;
    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
  }
}

} // namespace

TEST_SUITE("Array") {
  TEST_CASE("GrowsCopiesMovesAndDestroysElements") {
    CHECK_EQ(Probe::alive, 0);
    {
      auto values = Array<Probe>{};
      values.emplace_back(1);
      values.emplace_back(2);
      values.push_back(Probe{3});
      CHECK_EQ(values.size(), 3);
      CHECK_EQ(values[0].value, 1);
      CHECK_EQ(values[2].value, 3);
      CHECK_FALSE(values.empty());

      values.reserve(16);
      CHECK_EQ(Probe::alive, 3);
      CHECK_EQ(values[1].value, 2);

      auto listed = Array<Probe>{Probe{1}, Probe{2}, Probe{3}, Probe{4}, Probe{5}};
      CHECK_EQ(listed.size(), 5);
      CHECK_EQ(listed[4].value, 5);
      int seen = 0;
      for (const auto &item : listed) {
        seen += item.value;
      }
      CHECK_EQ(seen, 15);

      auto copy = listed;
      CHECK_EQ(Probe::alive, 3 + 5 + 5);
      copy[0].value = 9;
      CHECK_EQ(listed[0].value, 1);

      auto moved = move(listed);
      CHECK(listed.empty());
      CHECK_EQ(moved.size(), 5);
      CHECK_EQ(moved[4].value, 5);
      listed.emplace_back(7);
      CHECK_EQ(listed[0].value, 7);

      const auto &same = copy;
      copy = same;
      CHECK_EQ(copy.size(), 5);
      CHECK_EQ(copy[0].value, 9);

      copy = static_cast<Array<Probe> &&>(copy);
      CHECK_EQ(copy.size(), 5);
      CHECK_EQ(copy[1].value, 2);

      moved = copy;
      CHECK_EQ(moved.size(), 5);
      CHECK_EQ(moved[0].value, 9);
      CHECK_EQ(copy[0].value, 9);

      auto donor = Array<Probe>{};
      donor.emplace_back(8);
      moved = move(donor);
      CHECK(donor.empty());
      CHECK_EQ(moved.size(), 1);
      CHECK_EQ(moved[0].value, 8);

      moved.assign(2, Probe{5});
      CHECK_EQ(moved.size(), 2);
      CHECK_EQ(moved[0].value, 5);
      CHECK_EQ(moved[1].value, 5);

      moved = Array<Probe>{};
      CHECK(moved.empty());
      moved.clear();
      CHECK(moved.empty());
      moved.emplace_back(6);
      CHECK_EQ(moved[0].value, 6);

      Probe raw[]{Probe{1}, Probe{2}};
      auto  from = arrayFrom(raw, static_cast<u32>(sizeof raw / sizeof raw[0]));
      CHECK_EQ(from.size(), 2);
      CHECK_EQ(from[1].value, 2);
      auto none = arrayFrom<Probe>(nullptr, 0);
      CHECK(none.empty());
    }
    CHECK_EQ(Probe::alive, 0);
  }
}

TEST_SUITE("Maybe") {
  TEST_CASE("OwnsAValueUntilResetOrMove") {
    CHECK_EQ(Probe::alive, 0);
    {
      auto empty = Maybe<Probe>{};
      CHECK_FALSE(static_cast<bool>(empty));

      auto slot = Maybe<Probe>{Probe{4}};
      CHECK(static_cast<bool>(slot));
      CHECK_EQ(slot->value, 4);
      CHECK_EQ((*slot).value, 4);
      CHECK_EQ(Probe::alive, 1);

      slot.emplace(8);
      CHECK_EQ(slot->value, 8);
      CHECK_EQ(Probe::alive, 1);

      auto moved = Maybe<Probe>{move(slot)};
      CHECK_FALSE(static_cast<bool>(slot));
      CHECK(static_cast<bool>(moved));
      CHECK_EQ(moved->value, 8);
      CHECK_EQ(Probe::alive, 1);

      moved.reset();
      CHECK_FALSE(static_cast<bool>(moved));
      CHECK_EQ(Probe::alive, 0);
      moved.reset();
      CHECK_EQ(Probe::alive, 0);
    }
    CHECK_EQ(Probe::alive, 0);
  }
}

TEST_SUITE("Finite checks") {
  TEST_CASE_TEMPLATE("RejectsNaNAndInfinityButKeepsFiniteValues", T, f32, f64) {
    CHECK(is_finite(T{0}));
    CHECK(is_finite(-T{0}));
    CHECK(is_finite(std::numeric_limits<T>::max()));
    CHECK(is_finite(std::numeric_limits<T>::lowest()));
    CHECK(is_finite(std::numeric_limits<T>::min()));
    CHECK_FALSE(is_finite(std::numeric_limits<T>::infinity()));
    CHECK_FALSE(is_finite(-std::numeric_limits<T>::infinity()));
    CHECK_FALSE(is_finite(nan_of<T>()));
    CHECK_FALSE(is_finite(std::numeric_limits<T>::quiet_NaN()));
    CHECK_FALSE(is_finite(signalingNan<T>()));
  }
}

TEST_SUITE("wrapTwoPi") {
  TEST_CASE_TEMPLATE("WrapsExactTurnsAndNegativeAnglesIntoTheHalfOpenRange", T, f32, f64) {
    const T full    = math::kTwoPi<T>;
    const T quarter = full / T{4};
    CHECK_EQ(math::wrapTwoPi(T{0}), T{0});
    CHECK_EQ(math::wrapTwoPi(full), T{0});
    CHECK_EQ(math::wrapTwoPi(-full), T{0});
    CHECK_EQ(math::wrapTwoPi(full + full), T{0});
    CHECK_EQ(math::wrapTwoPi(-(full + full)), T{0});
    CHECK(math::wrapTwoPi(quarter) == doctest::Approx(quarter));
    CHECK(math::wrapTwoPi(-quarter) == doctest::Approx(full - quarter));
    CHECK(math::wrapTwoPi(full * T{2.25}) == doctest::Approx(quarter));
    CHECK(math::wrapTwoPi(full) < full);
    CHECK(math::wrapTwoPi(-quarter) >= T{0});
  }
}
