#include <doctest/doctest.h>

#include <base.hpp>
#include <core/hal.hpp>
#include <type_traits>

#include "../mock_runtime.hpp"

TEST_CASE_TEMPLATE("Host C bindings drive time, callbacks, GPIO and observations", T, f32, f64) {
  using Scope = std::conditional_t<std::is_same_v<T, f32>, push::f32::sinks::ScopeF32<>, push::f64::sinks::ScopeF64<>>;
  using Sine = std::conditional_t<std::is_same_v<T, f32>, push::f32::sources::SinGenF32<>, push::f64::sources::SinGenF64<>>;
  using Gpio = std::conditional_t<std::is_same_v<T, f32>, push::f32::sources::GpioInF32<>, push::f64::sources::GpioInF64<>>;

  MockRuntime::reset();
  Scope scope(0);
  Sine sine(1, 25);
  Gpio gpio(2, 7, {3});
  auto output = scope.apply({.channelCount = 2});
  sine.apply({.downstream = {output.channels[0]}});
  gpio.apply({.pins = {{output.channels[1]}}});

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
  CHECK_EQ(math::pow(T{2}, T{3}), T{8});

  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}
