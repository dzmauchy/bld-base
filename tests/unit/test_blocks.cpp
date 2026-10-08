#include <array>
#include <doctest/doctest.h>

#include <base/f32_blocks.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <ranges>
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

  TEST_CASE_FIXTURE(BlocksFixture, "StaysSilentUntilStartIncludingZero") {
    u32                       calls = 0;
    f32                       received = 1.f;
    core::function<void(f32)> receive = [&](const f32 value) {
      received = value;
      ++calls;
    };
    auto constant = ConstF32(1, 0.f);
    constant({.downstream = std::array{&receive}});

    MockRuntime::tick();
    MockRuntime::close();
    CHECK_EQ(calls, 0);

    MockRuntime::start();
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, 0.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    MockRuntime::start();
    CHECK_EQ(calls, 2);
    CHECK_EQ(received, 0.f);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RewireDropsThePreviousDownstream") {
    u32                       keptCalls = 0;
    u32                       replacedCalls = 0;
    f32                       replaced = 0.f;
    core::function<void(f32)> kept = [&](const f32) { ++keptCalls; };
    core::function<void(f32)> next = [&](const f32 value) {
      replaced = value;
      ++replacedCalls;
    };
    auto constant = ConstF32(1, -2.f);
    constant({.downstream = std::array{&kept}});
    constant({.downstream = std::array{&next}});

    MockRuntime::start();

    CHECK_EQ(keptCalls, 0);
    CHECK_GE(replacedCalls, 1);
    CHECK_EQ(replaced, -2.f);
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
    u32                       calls = 0;
    f32                       first = 0.f;
    f32                       second = 0.f;
    core::function<void(f32)> left = [&](const f32 value) {
      first = value;
      ++calls;
    };
    core::function<void(f32)> right = [&](const f32 value) {
      second = value;
      ++calls;
    };
    auto sine = SinF32(1);
    auto input = sine({.downstream = std::array<core::function<void(f32)> *, 3>{&left, nullptr, &right}})
                     .consumer;
    (*input)(-std::numbers::pi_v<f32> / 2.f);
    CHECK_EQ(calls, 2);
    CHECK(first == doctest::Approx(-1.f).epsilon(kEps));
    CHECK(second == doctest::Approx(-1.f).epsilon(kEps));

    (*input)(std::numeric_limits<f32>::quiet_NaN());
    CHECK_EQ(calls, 4);
    CHECK(std::isnan(first));
    CHECK(std::isnan(second));

    auto                      cosine = CosF32(2);
    f32                       cosineValue = 0.f;
    core::function<void(f32)> cosineSink = [&](const f32 value) { cosineValue = value; };
    auto                      cosineInput = cosine({.downstream = std::array{&cosineSink}}).consumer;
    (*cosineInput)(-1.25f);
    const auto negative = cosineValue;
    (*cosineInput)(1.25f);
    CHECK_EQ(cosineValue, negative);
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

  TEST_CASE_FIXTURE(BlocksFixture, "LatchesUntilTickAndDropsStaleOrNonFiniteResults") {
    u32                       calls = 0;
    f32                       received = -1.f;
    core::function<void(f32)> receive = [&](const f32 value) {
      received = value;
      ++calls;
    };
    auto output = ProductF32(1)({.downstream = std::array<core::function<void(f32)> *, 2>{&receive, nullptr}});
    auto inputs = output.channels(2);
    (*inputs[0])(3.f);
    (*inputs[1])(0.f);
    CHECK_EQ(calls, 0);

    MockRuntime::start();
    CHECK_EQ(calls, 0);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, 0.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 2);
    CHECK_EQ(received, 0.f);

    (*inputs[1])(4.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 3);
    CHECK_EQ(received, 12.f);

    (*inputs[0])(std::numeric_limits<f32>::max());
    (*inputs[1])(2.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 3);

    auto rebound = output.channels(2);
    MockRuntime::tick();
    CHECK_EQ(calls, 3);
    CHECK_EQ(MockRuntime::activeIntervalCount(), 1);
    (*rebound[0])(2.f);
    (*rebound[1])(5.f);
    MockRuntime::tick();
    CHECK_EQ(calls, 4);
    CHECK_EQ(received, 10.f);
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
    u32                       calls = 0;
    f32                       received = 1.f;
    core::function<void(f32)> receive = [&](const f32 value) {
      received = value;
      ++calls;
    };
    auto output = SumF32(1)({.downstream = std::array{&receive}});
    auto inputs = output.channels(2);
    (*inputs[0])(5.f);
    (*inputs[1])(-5.f);

    MockRuntime::start();
    CHECK_EQ(calls, 0);
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, 0.f);
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

  TEST_CASE_FIXTURE(BlocksFixture, "ResamplesAndKeepsTheAmplitudeSign") {
    u32                       calls = 0;
    f32                       received = 1.f;
    core::function<void(f32)> receive = [&](const f32 value) {
      received = value;
      ++calls;
    };
    auto gen = RandGenF32(1, 10, -2.f);
    gen({.downstream = std::array{&receive}});

    MockRuntime::setRandom(0.25f);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, -0.5f);

    MockRuntime::setRandom(0.5f);
    MockRuntime::tick();
    CHECK_EQ(calls, 2);
    CHECK_EQ(received, -1.f);

    auto                      zero = RandGenF32(2, 10, 0.f);
    u32                       zeroCalls = 0;
    f32                       zeroValue = 1.f;
    core::function<void(f32)> zeroSink = [&](const f32 value) {
      zeroValue = value;
      ++zeroCalls;
    };
    zero({.downstream = std::array{&zeroSink}});
    MockRuntime::start();
    MockRuntime::tick();
    CHECK_EQ(zeroCalls, 1);
    CHECK_EQ(zeroValue, 0.f);
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

  TEST_CASE_FIXTURE(BlocksFixture, "DutyEdgesAndNegativeAmplitude") {
    f32                       lowValue = -1.f;
    f32                       highValue = 0.f;
    f32                       windowValue = 1.f;
    core::function<void(f32)> lowSink = [&](const f32 value) { lowValue = value; };
    core::function<void(f32)> highSink = [&](const f32 value) { highValue = value; };
    core::function<void(f32)> windowSink = [&](const f32 value) { windowValue = value; };

    auto low = PulseGenF32(1, 0.f, 4.f);
    auto high = PulseGenF32(2, 1.f, -4.f);
    auto window = PulseGenF32(3, 0.5f, -4.f, 1.f);
    low({.downstream = std::array{&lowSink}});
    high({.downstream = std::array{&highSink}});
    window({.downstream = std::array{&windowSink}});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK_EQ(lowValue, 0.f);
    CHECK_EQ(highValue, -4.f);
    CHECK_EQ(windowValue, -4.f);

    MockRuntime::setNow(499);
    MockRuntime::tick();
    CHECK_EQ(lowValue, 0.f);
    CHECK_EQ(highValue, -4.f);
    CHECK_EQ(windowValue, -4.f);

    MockRuntime::setNow(500);
    MockRuntime::tick();
    CHECK_EQ(windowValue, 0.f);
    MockRuntime::setNow(999);
    MockRuntime::tick();
    CHECK_EQ(lowValue, 0.f);
    CHECK_EQ(highValue, -4.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ZeroFrequencyHoldsPhaseAndNegativeFrequencyReverses") {
    f32                       cosineValue = 0.f;
    f32                       sineValue = 0.f;
    core::function<void(f32)> cosineSink = [&](const f32 value) { cosineValue = value; };
    core::function<void(f32)> sineSink = [&](const f32 value) { sineValue = value; };
    auto                      cosine = CosGenF32(1, 10, 0.f, 3.f, std::numbers::pi_v<f32>);
    auto                      sine = SinGenF32(2, 10, -1.f, 1.f, 0.f);
    cosine({.downstream = std::array{&cosineSink}});
    sine({.downstream = std::array{&sineSink}});

    MockRuntime::setNow(0);
    MockRuntime::start();
    MockRuntime::tick();
    CHECK(cosineValue == doctest::Approx(-3.f).epsilon(kEps));
    CHECK(sineValue == doctest::Approx(0.f).epsilon(kEps));

    MockRuntime::setNow(250);
    MockRuntime::tick();
    CHECK(cosineValue == doctest::Approx(-3.f).epsilon(kEps));
    CHECK(sineValue == doctest::Approx(-1.f).epsilon(1e-4f));

    MockRuntime::setNow(5000);
    MockRuntime::tick();
    CHECK(cosineValue == doctest::Approx(-3.f).epsilon(kEps));
  }
}

TEST_SUITE("GpioInF32") {

  TEST_CASE_FIXTURE(BlocksFixture, "TrueIsOneOnScope") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1, 0, core::array<u8>(1, u8{0}));
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{sinks}}});

    MockRuntime::start();
    MockRuntime::emitGpio(0, 0, true);

    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "FalseIsZeroOnScope") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1);
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{sinks}}});

    MockRuntime::start();
    MockRuntime::emitGpio(0, 0, false);

    CHECK_EQ(MockRuntime::lastF32(0, 0), 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "IgnoresUnconfiguredPins") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(1);
    auto gpio = GpioInF32(1, 0, core::array<u8>{{2, 4}});
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{sinks, {}}}});

    MockRuntime::start();
    MockRuntime::emitGpio(0, 0, true);

    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "RoutesMultiplePins") {
    auto scope = ScopeF32(0);
    auto sinks = scope().channels(2);
    auto gpio = GpioInF32(1, 7, core::array<u8>{{1, 3}});
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{
              {std::array{sinks[0]}, std::array{sinks[1]}}}});

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
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{sinks}}});

    MockRuntime::start();
    CHECK_EQ(MockRuntime::activeGpioCount(), 1);
    MockRuntime::close();
    CHECK_EQ(MockRuntime::activeGpioCount(), 0);
    MockRuntime::emitGpio(0, 0, true);
    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "ListensThroughTheEighthPinOnly") {
    auto            scope = ScopeF32(0);
    auto            sinks = scope().channels(8);
    core::array<u8> pins(9);
    for (u8 index = 0; index < pins.size(); ++index)
      pins[index] = index;
    auto gpio = GpioInF32(1, 3, core::detail::move(pins));

    std::array<std::array<core::function<void(f32)> *, 1>, 8> slots{};
    core::array<VectorizedInput<core::function<void(f32)>>>   groups(8);
    for (u8 index = 0; index < groups.size(); ++index) {
      slots[index][0] = sinks[index];
      groups[index] = VectorizedInput<core::function<void(f32)>>{slots[index]};
    }
    gpio({.pins = groups});

    MockRuntime::emitGpio(3, 8, true);
    for (u8 index = 0; index < 8; ++index)
      CHECK_FALSE(MockRuntime::hasF32(0, index));
    MockRuntime::emitGpio(3, 0, true);
    MockRuntime::emitGpio(3, 7, false);
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1.f);
    CHECK_EQ(MockRuntime::lastF32(0, 7), 0.f);
    CHECK_FALSE(MockRuntime::hasF32(0, 1));
  }

  TEST_CASE_FIXTURE(BlocksFixture, "DuplicatePinFeedsOnlyTheFirstGroup") {
    u32                       firstCalls = 0;
    u32                       secondCalls = 0;
    f32                       first = -1.f;
    core::function<void(f32)> firstSink = [&](const f32 value) {
      first = value;
      ++firstCalls;
    };
    core::function<void(f32)> secondSink = [&](const f32) { ++secondCalls; };
    auto                      gpio = GpioInF32(1, 4, core::array<u8>{{3, 3}});
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{
              {std::array{&firstSink}, std::array{&secondSink}}}});

    MockRuntime::emitGpio(4, 3, true);

    CHECK_GT(firstCalls, 0);
    CHECK_EQ(first, 1.f);
    CHECK_EQ(secondCalls, 0);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SamePinOnAnotherPortStaysSeparate") {
    f32                       left = -1.f;
    f32                       right = -1.f;
    u32                       rightCalls = 0;
    core::function<void(f32)> leftSink = [&](const f32 value) { left = value; };
    core::function<void(f32)> rightSink = [&](const f32 value) {
      right = value;
      ++rightCalls;
    };
    auto first = GpioInF32(1, 1, core::array<u8>(1, u8{2}));
    auto second = GpioInF32(2, 2, core::array<u8>(1, u8{2}));
    first({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{std::array{&leftSink}}}});
    second(
        {.pins = core::array<VectorizedInput<core::function<void(f32)>>>{{std::array{&rightSink}}}});

    MockRuntime::emitGpio(1, 2, true);
    CHECK_EQ(left, 1.f);
    CHECK_EQ(rightCalls, 0);
    MockRuntime::emitGpio(2, 2, false);
    CHECK_EQ(left, 1.f);
    CHECK_EQ(right, 0.f);
  }

  TEST_CASE_FIXTURE(BlocksFixture, "SkipsNullConsumersAndPinsWithoutAGroup") {
    u32                       calls = 0;
    f32                       received = -1.f;
    core::function<void(f32)> receive = [&](const f32 value) {
      received = value;
      ++calls;
    };
    auto gpio = GpioInF32(1, 0, core::array<u8>{{5, 6, 7}});
    gpio({.pins = core::array<VectorizedInput<core::function<void(f32)>>>{
              {std::array<core::function<void(f32)> *, 2>{nullptr, &receive}}}});

    MockRuntime::emitGpio(0, 5, true);
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, 1.f);
    MockRuntime::emitGpio(0, 6, false);
    MockRuntime::emitGpio(0, 7, true);
    CHECK_EQ(calls, 1);
    CHECK_EQ(received, 1.f);
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
