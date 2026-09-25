#include <doctest/doctest.h>

#include <base.hpp>
#include <cmath>
#include <limits>

#include "../mock_runtime.hpp"

using push::f64::sinks::ScopeF64;
using push::f64::sources::ConstF64;
using push::f64::sources::CosGenF64;
using push::f64::sources::GpioInF64;
using push::f64::sources::PulseGenF64;
using push::f64::sources::RandGenF64;
using push::f64::sources::SinGenF64;
using push::f64::transformers::CosF64;
using push::f64::transformers::ProductF64;
using push::f64::transformers::SinF64;
using push::f64::transformers::SumF64;

namespace {

struct BlocksFixture {
  BlocksFixture() { MockRuntime::reset(); }
};

}  // namespace

TEST_SUITE("f64 blocks") {
  TEST_CASE_FIXTURE(BlocksFixture, "ConstFansOut") {
    auto scope = ScopeF64(0);
    auto sinks = scope.apply(2);
    auto constant = ConstF64(1, 8.0);
    constant.apply(sinks);

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF64(0, 0), 8.0);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 8.0);
    CHECK_EQ(constant.value(), 8.0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "CosThenSin") {
    auto scope = ScopeF64(0);
    auto sinks = scope.apply(1);
    auto sin = SinF64(1);
    auto cos = CosF64(2);
    auto sinInput = sin.apply(sinks);
    auto cosInput = cos.apply({sinInput});
    auto constant = ConstF64(3, 0.0);
    constant.apply({cosInput});

    MockRuntime::start();

    CHECK(MockRuntime::lastF64(0, 0) == doctest::Approx(std::sin(1.0)));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ProductAndSum") {
    auto scope = ScopeF64(0);
    auto sinks = scope.apply(2);
    auto product = ProductF64(1, 25);
    auto sum = SumF64(2);
    auto factors = product.apply({sinks[0]}, 2);
    auto terms = sum.apply({sinks[1]}, 2);
    auto a = ConstF64(3, 3.0);
    auto b = ConstF64(4, 4.0);
    a.apply({factors[0], terms[0]});
    b.apply({factors[1], terms[1]});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF64(0, 0), 12.0);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 7.0);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SumSkipsNonFinite") {
    auto scope = ScopeF64(0);
    auto sinks = scope.apply(1);
    auto sum = SumF64(1);
    auto inputs = sum.apply(sinks, 2);
    auto a = ConstF64(2, 6.0);
    auto b = ConstF64(3, std::numeric_limits<f64>::infinity());
    a.apply({inputs[0]});
    b.apply({inputs[1]});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_FALSE(MockRuntime::hasF64(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "WaveAndPulse") {
    auto scope = ScopeF64(0);
    auto sinks = scope.apply(3);
    auto cos = CosGenF64(1);
    auto sin = SinGenF64(2);
    auto pulse = PulseGenF64(3, 0.5);
    cos.apply({sinks[0]});
    sin.apply({sinks[1]});
    pulse.apply({sinks[2]});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF64(0, 0), 1.0);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 0.0);
    CHECK_EQ(MockRuntime::lastF64(0, 2), 1.0);

    MockRuntime::setNow(250);
    MockRuntime::tick();
    CHECK(MockRuntime::lastF64(0, 1) == doctest::Approx(1.0).epsilon(1e-9));

    MockRuntime::setNow(500);
    MockRuntime::tick();
    CHECK_EQ(MockRuntime::lastF64(0, 2), 0.0);
    CHECK_EQ(cos.precision(), 10);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RandomAndGpio") {
    auto scope = ScopeF64(0);
    auto sinks = scope.apply(2);
    auto rand = RandGenF64(1, 10, 2.0);
    auto gpio = GpioInF64(2, 7, {1});
    rand.apply({sinks[0]});
    gpio.connectPin(0, {sinks[1]});
    gpio.apply();

    MockRuntime::setRandom(0.25f);
    MockRuntime::start();
    MockRuntime::tick();
    MockRuntime::emitGpio(7, 1, true);

    CHECK_EQ(MockRuntime::lastF64(0, 0), 0.5);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 1.0);
    CHECK_EQ(MockRuntime::activeGpioCount(), 1);

    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
    CHECK_EQ(MockRuntime::activeGpioCount(), 0);
  }
}
