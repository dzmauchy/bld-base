#include <core/lib.hpp>

namespace {
int twice(int value) { return value * 2; }
struct alignas(128) Item {
  inline static int alive = 0;
  int               value;
  Item(int value = 0) : value(value) { ++alive; }
  Item(const Item &other) : Item(other.value) {}
  Item(Item &&other) noexcept : Item(other.value) { other.value = -1; }
  ~Item() { --alive; }
};
} // namespace

// Each failure reports its source line to the runner, without a testing runtime.
#define CHECK(expression)                                                                          \
  do {                                                                                             \
    if (!(expression))                                                                             \
      return __LINE__;                                                                             \
  } while (false)

extern "C" int core_lib_checks() {
  CHECK(Item::alive == 0);
  {
    core::function<int(int)> callback = [sum = 1](int value) mutable { return sum += value; };
    CHECK(callback(2) == 3);
    const auto copy = callback;
    CHECK(copy(4) == 7);
    CHECK(callback(1) == 4);
    auto moved = core::detail::move(callback);
    CHECK(!callback);
    CHECK(moved(2) == 6);
    moved = twice;
    CHECK(moved(5) == 10);
    moved = static_cast<int (*)(int)>(nullptr);
    CHECK(!moved);
    core::function<void(int)> discard = twice;
    discard(5);
    int                          value = 7;
    core::function<int &(int &)> reference = [](int &v) -> int & { return v; };
    reference(value) = 8;
    CHECK(value == 8);

    core::array<Item> values(2);
    values[0].value = 7;
    values[1].value = 7;
    CHECK(values[0].value == 7 && values[1].value == 7);
    CHECK(values.back().value == 7);
    CHECK(reinterpret_cast<core::size_t>(values.data()) % alignof(Item) == 0);
    CHECK(Item::alive == 2);
    {
      auto copied = values;
      copied[0].value = 4;
      CHECK(values[0].value == 7);
      auto *storage = copied.data();
      auto  transferred = core::detail::move(copied);
      CHECK(copied.empty());
      CHECK(transferred.front().value == 4);
      CHECK(transferred.data() == storage);
    }
    CHECK(Item::alive == 2);

    core::array<core::function<int()>> callbacks(101);
    callbacks[0] = [item = Item{9}] { return item.value; };
    for (int i = 0; i < 100; ++i)
      callbacks[i + 1] = [i] { return i; };
    auto copiedCallbacks = callbacks;
    CHECK(callbacks[0]() == 9 && copiedCallbacks[0]() == 9);
    CHECK(callbacks.back()() == 99);
    callbacks[0] = nullptr;
    CHECK(Item::alive == 3);
    core::array<int> literal{{2, 3, 5}};
    CHECK(literal.size() == 3 && literal[2] == 5);
    core::array<int> zeros(5);
    CHECK(zeros[4] == 0);
  }
  CHECK(Item::alive == 0);
  return 0;
}

extern "C" void core_lib_empty_function() { core::function<void()>{}(); }
extern "C" void core_lib_invalid_index() { core::array<int>{}.at(0); }
extern "C" void core_lib_size_overflow() {
  core::array<int> values(core::array<int>::max_size() + 1);
}
