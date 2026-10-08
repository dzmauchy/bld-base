#include <array>
#include <doctest/doctest.h>

#include <base/f64_blocks.hpp>
#include <cmath>
#include <limits>
#include <numbers>

#include "../mock_runtime.hpp"

using push::f_64::sinks::ScopeF64;
using push::f_64::sources::ConstF64;
using push::f_64::sources::CosGenF64;
using push::f_64::sources::GpioInF64;
using push::f_64::sources::PulseGenF64;
using push::f_64::sources::RandGenF64;
using push::f_64::sources::SinGenF64;
using push::f_64::transformers::CosF64;
using push::f_64::transformers::ProductF64;
using push::f_64::transformers::SinF64;
using push::f_64::transformers::SumF64;

namespace {

struct BlocksFixture {
  BlocksFixture() { MockRuntime::reset(); }
};

} // namespace

TEST_SUITE("f64 blocks") {
  TEST_CASE_FIXTURE(BlocksFixture, "ConstFansOut") {
    auto scope = ScopeF64(0);
    auto sinks = scope().channels(2);
    auto constant = ConstF64(1, 8.0);
    constant({.downstream = sinks});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF64(0, 0), 8.0);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 8.0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "CosThenSin") {
    auto scope = ScopeF64(0);
    auto sinks = scope().channels(1);
    auto sin = SinF64(1);
    auto cos = CosF64(2);
    auto sinInput = sin({.downstream = sinks}).consumer;
    auto cosInput = cos({.downstream = std::array{sinInput}}).consumer;
    auto constant = ConstF64(3, 0.0);
    constant({.downstream = std::array{cosInput}});

    MockRuntime::start();

    CHECK(MockRuntime::lastF64(0, 0) == doctest::Approx(std::sin(1.0)));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ProductAndSum") {
    auto scope = ScopeF64(0);
    auto sinks = scope().channels(2);
    auto product = ProductF64(1, 25);
    auto sum = SumF64(2);
    auto factors = product({.downstream = std::array{sinks[0]}}).channels(2);
    auto terms = sum({.downstream = std::array{sinks[1]}}).channels(2);
    auto a = ConstF64(3, 3.0);
    auto b = ConstF64(4, 4.0);
    a({.downstream = std::array{factors[0], terms[0]}});
    b({.downstream = std::array{factors[1], terms[1]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF64(0, 0), 12.0);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 7.0);
    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SumSkipsNonFinite") {
    auto scope = ScopeF64(0);
    auto sinks = scope().channels(1);
    auto sum = SumF64(1);
    auto inputs = sum({.downstream = sinks}).channels(2);
    auto a = ConstF64(2, 6.0);
    auto b = ConstF64(3, std::numeric_limits<f64>::infinity());
    a({.downstream = std::array{inputs[0]}});
    b({.downstream = std::array{inputs[1]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_FALSE(MockRuntime::hasF64(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "WaveAndPulse") {
    auto scope = ScopeF64(0);
    auto sinks = scope().channels(3);
    auto cos = CosGenF64(1);
    auto sin = SinGenF64(2);
    auto pulse = PulseGenF64(3, 0.5);
    cos({.downstream = std::array{sinks[0]}});
    sin({.downstream = std::array{sinks[1]}});
    pulse({.downstream = std::array{sinks[2]}});

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
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RandomAndGpio") {
    auto scope = ScopeF64(0);
    auto sinks = scope().channels(2);
    auto rand = RandGenF64(1, 10, 2.0);
    auto gpio = GpioInF64(2, 7, core::array<u8>(1, u8{1}));
    rand({.downstream = std::array{sinks[0]}});
    gpio({.pins = core::array<VectorizedInput<core::function<void(f64)>>>{{std::array{sinks[1]}}}});

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

  TEST_CASE_FIXTURE(BlocksFixture, "ProductLatchAndZeroFrequencyPhase") {
    u32                       calls = 0;
    f64                       received = 1;
    core::function<void(f64)> receive = [&](const f64 value) {
      received = value;
      ++calls;
    };
    auto product = ProductF64(1);
    auto inputs = product({.downstream = std::array{&receive}}).channels(2);
    (*inputs[0])(0);
    (*inputs[1])(8);
    MockRuntime::start();
    CHECK_EQ(calls, 0);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, 0);

    (*inputs[0])(std::numeric_limits<f64>::max());
    (*inputs[1])(2);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);

    f64                       wave = 0;
    core::function<void(f64)> waveSink = [&](const f64 value) { wave = value; };
    auto                      cosine = CosGenF64(2, 10, 0, 3, std::numbers::pi_v<f64>);
    cosine({.downstream = std::array{&waveSink}});
    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK(wave == doctest::Approx(-3));
    MockRuntime::setNow(5000);
    MockRuntime::tick();
    CHECK(wave == doctest::Approx(-3));

    auto sine = SinF64(3);
    f64  sineValue = 0;
    core::function<void(f64)> sineSink = [&](const f64 value) { sineValue = value; };
    (*sine({.downstream = std::array{&sineSink}}).consumer)(-std::numbers::pi_v<f64> / 2);
    CHECK(sineValue == doctest::Approx(-1));
  }
}
