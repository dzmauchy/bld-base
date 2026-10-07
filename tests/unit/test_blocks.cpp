#include <array>
#include <doctest/doctest.h>

#include <base/f32_blocks.hpp>
#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <ranges>
#include <span>
#include <vector>

#include "../mock_runtime.hpp"

using push::f_32::sinks::ScopeF32;
using push::f_32::sources::ConstF32;
using push::f_32::sources::CosGenF32;
using push::f_32::sources::GpioInF32;
using push::f_32::sources::PulseGenF32;
using push::f_32::sources::RandGenF32;
using push::f_32::sources::SinGenF32;
using push::f_32::transformers::CosF32;
using push::f_32::transformers::ProductF32;
using push::f_32::transformers::SinF32;
using push::f_32::transformers::SumF32;

namespace {

struct BlocksFixture {
  BlocksFixture() { MockRuntime::reset(); }
};

constexpr f32 kEps = 1e-5f;

} // namespace

TEST_SUITE("ScopeF32") {
  TEST_CASE_FIXTURE(BlocksFixture, "DoesNotReportAValueBeforeAnyPush") {
    auto scope = ScopeF32(0);
    (void)scope().channels(1);

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_FALSE(MockRuntime::hasF32(0, 0));
    CHECK(std::isnan(MockRuntime::lastF32(0, 0)));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ChannelsAreIndependent") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto a = ConstF32(1, 1.5f);
    auto b = ConstF32(2, 9.5f);
    a({.downstream = std::array{sinks[0]}});
    b({.downstream = std::array{sinks[1]}});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.5f);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 9.5f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ReportsToConfiguredBlockId") {
    const auto scope = ScopeF32(3, 120, 25);
    auto       channels = scope().channels(1);
    (*channels[0])(2.f);
    CHECK_EQ(MockRuntime::lastF32(3, 0), 2.f);
  }
}

TEST_SUITE("ConstF32") {
  TEST_CASE_FIXTURE(BlocksFixture, "PushesValueToScope") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto constant = ConstF32(1, 3.5f);
    constant({.downstream = sinks});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 3.5f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "FansOutToTwoScopeChannels") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto constant = ConstF32(1, 8.f);
    constant({.downstream = sinks});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 8.f);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 8.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "DefaultValueIsOne") {
    auto scope = ScopeF32(0);
    auto constant = ConstF32(1);
    constant({.downstream = scope().channels(1)});
    MockRuntime::start();
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "DoesNotRegisterAnInterval") {
    auto constant = ConstF32(1, 9.f);
    constant({.downstream = {}});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "StaysSilentUntilStartThenReplacesDownstream") {
    u32                      staleCalls = 0;
    u32                      calls = 0;
    f32                      last = 1.f;
    std::function<void(f32)> stale = [&staleCalls](f32) { ++staleCalls; };
    std::function<void(f32)> receive = [&calls, &last](const f32 value) {
      ++calls;
      last = value;
    };
    auto constant = ConstF32(1, 0.f);
    constant({.downstream = std::array{&stale}});

    MockRuntime::tick();
    MockRuntime::close();
    CHECK_EQ(staleCalls, 0);

    MockRuntime::start();
    CHECK_EQ(staleCalls, 1);
    MockRuntime::tick();
    CHECK_EQ(staleCalls, 1);

    constant({.downstream = std::array{&receive}});
    MockRuntime::start();
    CHECK_EQ(staleCalls, 1);
    CHECK(calls >= 1);
    CHECK_EQ(last, 0.f);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "EmitsAgainWhenTheRuntimeStartsAgain") {
    u32                      calls = 0;
    f32                      last = 0.f;
    std::function<void(f32)> receive = [&calls, &last](const f32 value) {
      ++calls;
      last = value;
    };
    auto constant = ConstF32(1, -2.5f);
    constant({.downstream = std::array{&receive}});

    MockRuntime::start();
    CHECK_EQ(calls, 1);
    CHECK_EQ(last, -2.5f);

    MockRuntime::start();
    CHECK_EQ(calls, 2);
    CHECK_EQ(last, -2.5f);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }
}

TEST_SUITE("UnaryTransformers") {
  TEST_CASE_FIXTURE(BlocksFixture, "CosOfZeroIsOne") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto cos = CosF32(1);
    auto input = cos({.downstream = sinks}).consumer;
    auto constant = ConstF32(2, 0.f);
    constant({.downstream = std::array{input}});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SinOfZeroIsZero") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto sin = SinF32(1);
    auto input = sin({.downstream = sinks}).consumer;
    auto constant = ConstF32(2, 0.f);
    constant({.downstream = std::array{input}});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "CosThenSinOfZeroIsSinOfOne") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto sin = SinF32(1);
    auto cos = CosF32(2);
    auto sinInput = sin({.downstream = sinks}).consumer;
    auto cosInput = cos({.downstream = std::array{sinInput}}).consumer;
    auto constant = ConstF32(3, 0.f);
    constant({.downstream = std::array{cosInput}});

    MockRuntime::start();

    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(std::sin(1.f)).epsilon(kEps));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ForwardsNonFiniteValuesAndSkipsNullSinks") {
    u32                      leftCalls = 0;
    u32                      rightCalls = 0;
    f32                      left = 0.f;
    f32                      right = 0.f;
    std::function<void(f32)> leftSink = [&leftCalls, &left](const f32 value) {
      ++leftCalls;
      left = value;
    };
    std::function<void(f32)> rightSink = [&rightCalls, &right](const f32 value) {
      ++rightCalls;
      right = value;
    };
    auto sine = SinF32(1);
    auto input = sine({.downstream = std::array<std::function<void(f32)> *, 3>{
                           &leftSink, nullptr, &rightSink}})
                     .consumer;

    (*input)(std::numeric_limits<f32>::quiet_NaN());
    CHECK_EQ(leftCalls, 1);
    CHECK_EQ(rightCalls, 1);
    CHECK(std::isnan(left));
    CHECK(std::isnan(right));

    (*input)(-std::numbers::pi_v<f32> / 2.f);
    CHECK_EQ(leftCalls, 2);
    CHECK_EQ(rightCalls, 2);
    CHECK(left == doctest::Approx(-1.f).epsilon(kEps));
    CHECK(right == doctest::Approx(-1.f).epsilon(kEps));
  }
}

TEST_SUITE("ProductF32") {
  TEST_CASE_FIXTURE(BlocksFixture, "MultipliesTwoConstants") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto product = ProductF32(1);
    auto inputs = product({.downstream = sinks}).channels(2);
    auto a = ConstF32(2, 3.f);
    auto b = ConstF32(3, 4.f);
    a({.downstream = std::array{inputs[0]}});
    b({.downstream = std::array{inputs[1]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 12.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SingleFactorIsTheProduct") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto product = ProductF32(1);
    auto inputs = product({.downstream = sinks}).channels(1);
    auto a = ConstF32(2, 6.f);
    a({.downstream = std::array{inputs[0]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 6.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "DoesNotPushWhenAFactorIsNaN") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto product = ProductF32(1);
    auto inputs = product({.downstream = sinks}).channels(2);
    auto a = ConstF32(2, 6.f);
    a({.downstream = std::array{inputs[0]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "UsesConfiguredPrecision") {
    auto product = ProductF32(1, 25);
    (void)product({.downstream = {}}).channels(1);

    MockRuntime::start();

    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "LatchesTheLatestFiniteFactorsAndRepublishesThem") {
    u32                      staleCalls = 0;
    u32                      leftCalls = 0;
    u32                      rightCalls = 0;
    f32                      left = -1.f;
    f32                      right = -1.f;
    std::function<void(f32)> stale = [&staleCalls](f32) { ++staleCalls; };
    std::function<void(f32)> leftSink = [&leftCalls, &left](const f32 value) {
      ++leftCalls;
      left = value;
    };
    std::function<void(f32)> rightSink = [&rightCalls, &right](const f32 value) {
      ++rightCalls;
      right = value;
    };
    auto product = ProductF32(1, 10);
    (void)product({.downstream = std::array{&stale}});
    auto inputs = product({.downstream = std::array<std::function<void(f32)> *, 3>{
                               &leftSink, nullptr, &rightSink}})
                      .channels(2);
    (*inputs[0])(0.f);
    (*inputs[1])(5.f);

    MockRuntime::start();
    CHECK_EQ(leftCalls, 0);
    CHECK_EQ(rightCalls, 0);
    MockRuntime::tick();
    CHECK_EQ(staleCalls, 0);
    CHECK_EQ(leftCalls, 1);
    CHECK_EQ(rightCalls, 1);
    CHECK_EQ(left, 0.f);
    CHECK_EQ(right, 0.f);

    (*inputs[0])(3.f);
    MockRuntime::tick();
    CHECK_EQ(leftCalls, 2);
    CHECK_EQ(left, 15.f);
    CHECK_EQ(right, 15.f);

    MockRuntime::tick();
    CHECK_EQ(leftCalls, 3);
    CHECK_EQ(rightCalls, 3);
    CHECK_EQ(left, 15.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SuppressesANonFiniteProduct") {
    u32                      calls = 0;
    std::function<void(f32)> receive = [&calls](f32) { ++calls; };
    auto                     product = ProductF32(1);
    auto                     inputs = product({.downstream = std::array{&receive}}).channels(2);
    (*inputs[0])(std::numeric_limits<f32>::max());
    (*inputs[1])(2.f);

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(calls, 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RebindDropsStaleFactorsUntilReplaced") {
    u32                      calls = 0;
    f32                      last = 0.f;
    std::function<void(f32)> receive = [&calls, &last](const f32 value) {
      ++calls;
      last = value;
    };
    auto product = ProductF32(1, 10);
    auto output = product({.downstream = std::array{&receive}});
    auto inputs = output.channels(2);
    (*inputs[0])(3.f);
    (*inputs[1])(4.f);

    MockRuntime::start();
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(last, 12.f);

    auto rebound = output.channels(2);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);

    (*rebound[0])(2.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);

    (*rebound[1])(3.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 2);
    CHECK_EQ(last, 6.f);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
  }
}

TEST_SUITE("SumF32") {
  TEST_CASE_FIXTURE(BlocksFixture, "AddsTwoConstants") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto sum = SumF32(1);
    auto inputs = sum({.downstream = sinks}).channels(2);
    auto a = ConstF32(2, 3.f);
    auto b = ConstF32(3, 4.f);
    a({.downstream = std::array{inputs[0]}});
    b({.downstream = std::array{inputs[1]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 7.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "DoesNotPushWhenATermIsNonFinite") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto sum = SumF32(1);
    auto inputs = sum({.downstream = sinks}).channels(2);
    auto a = ConstF32(2, 6.f);
    auto b = ConstF32(3, std::numeric_limits<f32>::infinity());
    a({.downstream = std::array{inputs[0]}});
    b({.downstream = std::array{inputs[1]}});

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "PublishesAnExactZeroSum") {
    u32                      calls = 0;
    f32                      last = 1.f;
    std::function<void(f32)> receive = [&calls, &last](const f32 value) {
      ++calls;
      last = value;
    };
    auto sum = SumF32(1);
    auto inputs = sum({.downstream = std::array{&receive}}).channels(2);
    (*inputs[0])(5.f);
    (*inputs[1])(-5.f);

    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(calls, 1);
    CHECK_EQ(last, 0.f);
  }
}

TEST_SUITE("WaveGenerators") {
  TEST_CASE_FIXTURE(BlocksFixture, "CosGenAtZeroIsOne") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = CosGenF32(1);
    gen({.downstream = sinks});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SinGenAtZeroIsZero") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = SinGenF32(1);
    gen({.downstream = sinks});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SinGenAtQuarterPeriodIsOne") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = SinGenF32(1);
    gen({.downstream = sinks});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::setNow(250);
    MockRuntime::tick();

    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(1.f).epsilon(1e-4f));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "CosGenUsesConfiguredPrecision") {
    auto gen = CosGenF32(1, 25);
    gen({.downstream = {}});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 25);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "OnCloseClearsGeneratorInterval") {
    auto gen = CosGenF32(1);
    gen({.downstream = {}});

    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SinGenScalesFrequencyAndNegativeAmplitude") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = SinGenF32(1, 10, 2.f, -3.f, 0.f);
    gen({.downstream = sinks});

    MockRuntime::setNow(0);
    MockRuntime::tick();
    CHECK_FALSE(MockRuntime::hasF32(0, 0));

    MockRuntime::start();
    MockRuntime::setNow(125);
    MockRuntime::tick();

    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(-3.f).epsilon(1e-4f));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ZeroFrequencyHoldsThePhaseSample") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = CosGenF32(1, 10, 0.f, 2.f, std::numbers::pi_v<f32>);
    gen({.downstream = sinks});

    MockRuntime::setNow(100);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(-2.f).epsilon(1e-4f));

    MockRuntime::setNow(350);
    MockRuntime::tick();
    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(-2.f).epsilon(1e-4f));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RetargetKeepsTheStartTimeAndOneInterval") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto gen = SinGenF32(1);
    gen({.downstream = std::array{sinks[0]}});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);

    MockRuntime::setNow(250);
    gen({.downstream = std::array{sinks[1]}});
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
    CHECK(MockRuntime::lastF32(0, 1) == doctest::Approx(1.f).epsilon(1e-4f));
  }
}

TEST_SUITE("RandGenF32") {
  TEST_CASE_FIXTURE(BlocksFixture, "UsesInjectedRandomScaledByAmplitude") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = RandGenF32(1, 10, 2.f);
    gen({.downstream = sinks});

    MockRuntime::setRandom(0.25f);
    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.5f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ResamplesEachTickAndAppliesNonPositiveAmplitude") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto negative = RandGenF32(1, 10, -2.f);
    auto zero = RandGenF32(2, 10, 0.f);
    negative({.downstream = std::array{sinks[0]}});
    zero({.downstream = std::array{sinks[1]}});

    MockRuntime::setRandom(0.25f);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK_EQ(MockRuntime::lastF32(0, 0), -0.5f);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 0.f);

    MockRuntime::setRandom(0.5f);
    MockRuntime::tick();
    CHECK_EQ(MockRuntime::lastF32(0, 0), -1.f);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 0.f);
  }
}

TEST_SUITE("PulseGenF32") {
  TEST_CASE_FIXTURE(BlocksFixture, "HighAtStartOfPeriod") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = PulseGenF32(1, 0.5f);
    gen({.downstream = sinks});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "LowAfterDutyWindow") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gen = PulseGenF32(1, 0.5f);
    gen({.downstream = sinks});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::setNow(500);
    MockRuntime::tick();

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "UsesOneMillisecondInterval") {
    auto gen = PulseGenF32(1);
    gen({.downstream = {}});

    MockRuntime::start();

    CHECK_EQ(MockRuntime::intervalPeriodAt(0), 1);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "NegativeAmplitudeRepeatsWhileHighAndHonorsDutyEdges") {
    u32                      highCalls = 0;
    f32                      highLast = 1.f;
    u32                      zeroCalls = 0;
    f32                      zeroLast = 1.f;
    u32                      fullCalls = 0;
    f32                      fullLast = 0.f;
    std::function<void(f32)> high = [&highCalls, &highLast](const f32 value) {
      ++highCalls;
      highLast = value;
    };
    std::function<void(f32)> zeroDutySink = [&zeroCalls, &zeroLast](const f32 value) {
      ++zeroCalls;
      zeroLast = value;
    };
    std::function<void(f32)> fullDutySink = [&fullCalls, &fullLast](const f32 value) {
      ++fullCalls;
      fullLast = value;
    };
    auto negative = PulseGenF32(1, 0.5f, -4.f, 2.f);
    auto zeroDuty = PulseGenF32(2, 0.f, 3.f);
    auto fullDuty = PulseGenF32(3, 1.f, 3.f, 2.f);
    negative({.downstream = std::array{&high}});
    zeroDuty({.downstream = std::array{&zeroDutySink}});
    fullDuty({.downstream = std::array{&fullDutySink}});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();
    MockRuntime::tick();
    CHECK_EQ(highCalls, 2);
    CHECK_EQ(highLast, -4.f);
    CHECK_EQ(zeroCalls, 2);
    CHECK_EQ(zeroLast, 0.f);
    CHECK_EQ(fullCalls, 2);
    CHECK_EQ(fullLast, 3.f);

    MockRuntime::setNow(250);
    MockRuntime::tick();
    CHECK_EQ(highLast, 0.f);
    CHECK_EQ(zeroLast, 0.f);
    CHECK_EQ(fullLast, 3.f);
  }
}

TEST_SUITE("GpioInF32") {

  TEST_CASE_FIXTURE(BlocksFixture, "TrueIsOneOnScope") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1, 0, {0});
    gpio({.pins = {sinks}});

    MockRuntime::start();
    MockRuntime::emitGpio(0, 0, true);

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "FalseIsZeroOnScope") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1);
    gpio({.pins = {sinks}});

    MockRuntime::start();
    MockRuntime::emitGpio(0, 0, false);

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "IgnoresUnconfiguredPins") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1, 0, {2, 4});
    gpio({.pins = {sinks, {}}});

    MockRuntime::start();
    MockRuntime::emitGpio(0, 0, true);

    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RoutesMultiplePins") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto gpio = GpioInF32(1, 7, {1, 3});
    gpio({.pins = {std::array{sinks[0]}, std::array{sinks[1]}}});

    MockRuntime::start();
    MockRuntime::emitGpio(7, 1, true);
    MockRuntime::emitGpio(7, 3, false);

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "OnCloseStopsListening") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1);
    gpio({.pins = {sinks}});

    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeGpioCount(), 1);
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeGpioCount(), 0);
    MockRuntime::emitGpio(0, 0, true);
    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "CapsListenersAtEightPins") {
    auto            scope = ScopeF32(0);
    auto            channels = scope().channels(9);
    std::vector<u8> pins(9);
    std::vector<std::array<std::function<void(f32)> *, 1>> storage(9);
    std::vector<std::span<std::function<void(f32)> *const>> groups;
    groups.reserve(9);
    for (u8 i = 0; i < 9; ++i) {
      pins[i] = i;
      storage[i][0] = channels[i];
      groups.emplace_back(storage[i]);
    }
    auto gpio = GpioInF32(1, 2, std::move(pins));
    gpio({.pins = groups});

    CHECK_EQ(MockRuntime::activeGpioCount(), 8);
    MockRuntime::emitGpio(2, 7, true);
    CHECK_EQ(MockRuntime::lastF32(0, 7), 1.f);
    MockRuntime::emitGpio(2, 8, true);
    CHECK_FALSE(MockRuntime::hasF32(0, 8));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ShortConsumerListSkipsPinsWithoutAGroup") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1, 0, {1, 2, 3});
    gpio({.pins = {sinks}});

    CHECK_EQ(MockRuntime::activeGpioCount(), 3);
    MockRuntime::emitGpio(0, 1, true);
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
    MockRuntime::emitGpio(0, 3, false);
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "DuplicatePinFeedsOnlyTheFirstGroup") {
    u32                      firstCalls = 0;
    u32                      secondCalls = 0;
    std::function<void(f32)> first = [&firstCalls](f32) { ++firstCalls; };
    std::function<void(f32)> second = [&secondCalls](f32) { ++secondCalls; };
    auto                     gpio = GpioInF32(1, 4, {7, 7});
    gpio({.pins = {std::array{&first}, std::array{&second}}});

    MockRuntime::emitGpio(4, 7, true);

    CHECK(firstCalls > 0);
    CHECK_EQ(secondCalls, 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SamePinOnAnotherPortStaysIsolated") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto left = GpioInF32(1, 1, {4});
    auto right = GpioInF32(2, 2, {4});
    left({.pins = {std::array{sinks[0]}}});
    right({.pins = {std::array{sinks[1]}}});

    MockRuntime::emitGpio(1, 4, true);
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
    CHECK_FALSE(MockRuntime::hasF32(0, 1));

    MockRuntime::emitGpio(2, 4, false);
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "NullConsumerInAGroupIsSkipped") {
    u32                      calls = 0;
    f32                      last = 0.f;
    std::function<void(f32)> receive = [&calls, &last](const f32 value) {
      ++calls;
      last = value;
    };
    std::array<std::function<void(f32)> *, 2> group{nullptr, &receive};
    auto                                      gpio = GpioInF32(1, 0, {2});
    gpio({.pins = {group}});

    MockRuntime::emitGpio(0, 2, true);

    CHECK_EQ(calls, 1);
    CHECK_EQ(last, 1.f);
  }
}

TEST_SUITE("CompositeDiagrams") {
  TEST_CASE_FIXTURE(BlocksFixture, "CosTimesSinAtQuarterPeriod") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto product = ProductF32(1);
    auto factors = product({.downstream = sinks}).channels(2);
    auto cos = CosGenF32(2);
    auto sin = SinGenF32(3);
    cos({.downstream = std::array{factors[0]}});
    sin({.downstream = std::array{factors[1]}});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::setNow(250);
    MockRuntime::tick();
    MockRuntime::tick();

    CHECK(MockRuntime::lastF32(0, 0) == doctest::Approx(0.f).epsilon(1e-4f));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "WiredCallablesSurviveVectorReallocation") {
    constexpr auto kCount = u8{70};
    auto           scope = ScopeF32(0);
    auto           sinks = scope().channels(kCount);
    auto           constants = std::vector<decltype(ConstF32(0))>{};
    constants.reserve(1);
    for (auto i : std::views::iota(u8{}, kCount)) {
      constants.push_back(ConstF32(static_cast<u32>(i) + 1, static_cast<f32>(i)));
      constants.back()({.downstream = std::array{sinks[i]}});
    }

    MockRuntime::start();

    for (auto i : std::views::iota(u8{}, kCount)) {
      CHECK_EQ(MockRuntime::lastF32(0, i), static_cast<f32>(i));
    }
  }
}
