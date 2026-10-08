#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <core/diagram.hpp>
#include <doctest/doctest.h>
#include <type_traits>

#include "../mock_runtime.hpp"

namespace {
template <typename T> auto scopeFactory() {
  if constexpr (std::is_same_v<T, f32>)
    return push::f_32::sinks::ScopeF32;
  else
    return push::f_64::sinks::ScopeF64;
}
template <typename T> auto constantFactory() {
  if constexpr (std::is_same_v<T, f32>)
    return push::f_32::sources::ConstF32;
  else
    return push::f_64::sources::ConstF64;
}
template <typename T> auto gpioFactory() {
  if constexpr (std::is_same_v<T, f32>)
    return push::f_32::sources::GpioInF32;
  else
    return push::f_64::sources::GpioInF64;
}
template <typename T> auto cosineFactory() {
  if constexpr (std::is_same_v<T, f32>)
    return push::f_32::transformers::CosF32;
  else
    return push::f_64::transformers::CosF64;
}
void configurationFactory(u32,
                          i32,
                          Bool,
                          core::array<u8>) noexcept {}
} // namespace

TEST_CASE("Factory configuration uses parameter positions and preserves array elements") {
  CHECK_EQ(core::config_arg<0>(configurationFactory, 3.75), 3);
  CHECK(core::config_arg<1>(configurationFactory, true));
  const auto pins = core::config_arg<2>(configurationFactory, 1, 3, 7);
  REQUIRE_EQ(pins.size(), 3);
  CHECK_EQ(pins[0], 1);
  CHECK_EQ(pins[1], 3);
  CHECK_EQ(pins[2], 7);
  CHECK(core::config_arg<2>(configurationFactory).empty());
}

TEST_CASE_TEMPLATE("Uniform diagram calls wire fanout and copy temporary connection lists",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  auto scope = scopeFactory<T>()(0, 60, 10);
  auto constant = constantFactory<T>()(1, core::config_arg<0>(constantFactory<T>(), 3.5));
  auto output = core::bind_block(scope, core::block_inputs(scope));
  auto channels = core::output_channels<true, 2>(output.channels);
  {
    auto input = core::block_inputs(constant);
    auto connections = core::input_connections<true, 2, 2>(input.downstream);
    connections.connect(0, channels.at(0));
    connections.connect(1, channels.at(1));
    input.downstream = connections.view();
    auto unusedOutput = core::bind_block(constant, core::detail::move(input));
    (void)unusedOutput;
  }
  MockRuntime::start();
  if constexpr (std::is_same_v<T, f32>) {
    CHECK_EQ(MockRuntime::lastF32(0, 0), 3.5);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 3.5);
  } else {
    CHECK_EQ(MockRuntime::lastF64(0, 0), 3.5);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 3.5);
  }
  MockRuntime::close();
}

TEST_CASE_TEMPLATE("Grouped ports preserve holes, fanout, configuration, and callback ownership",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  auto scope = scopeFactory<T>()(0, 60, 10);
  auto output = core::bind_block(scope, core::block_inputs(scope));
  auto channels = core::output_channels<true, 2>(output.channels);
  auto gpio = gpioFactory<T>()(1, core::config_arg<0>(gpioFactory<T>(), 7),
                               core::config_arg<1>(gpioFactory<T>(), 1, 3));
  CHECK_EQ(MockRuntime::registeredGpioCount(), 1);
  CHECK_EQ(MockRuntime::registeredGpioPort(1), 7);
  REQUIRE_EQ(MockRuntime::registeredGpioPinCount(1), 2);
  CHECK_EQ(MockRuntime::registeredGpioPin(1, 0), 1);
  CHECK_EQ(MockRuntime::registeredGpioPin(1, 1), 3);
  {
    auto input = core::block_inputs(gpio);
    auto connections = core::input_connections<true, 2, 2>(input.pins);
    connections.connect(1, channels.at(0));
    connections.connect(1, channels.at(1));
    input.pins = connections.view();
    core::bind_block(gpio, core::detail::move(input));
  }
  MockRuntime::emitGpio(7, 1, true);
  if constexpr (std::is_same_v<T, f32>)
    CHECK_FALSE(MockRuntime::hasF32(0, 0));
  else
    CHECK_FALSE(MockRuntime::hasF64(0, 0));
  MockRuntime::emitGpio(7, 3, true);
  if constexpr (std::is_same_v<T, f32>) {
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1);
    CHECK_EQ(MockRuntime::lastF32(0, 1), 1);
  } else {
    CHECK_EQ(MockRuntime::lastF64(0, 0), 1);
    CHECK_EQ(MockRuntime::lastF64(0, 1), 1);
  }
  MockRuntime::close();
  CHECK_EQ(MockRuntime::activeGpioCount(), 0);
}

TEST_CASE_TEMPLATE("Scalar consumer outputs connect to vectorized inputs without type inspection",
                   T,
                   f32,
                   f64) {
  MockRuntime::reset();
  auto scope = scopeFactory<T>()(0, 60, 10);
  auto cosine = cosineFactory<T>()(1);
  auto constant = constantFactory<T>()(2, core::config_arg<0>(constantFactory<T>(), 0));
  auto scopeOutput = core::bind_block(scope, core::block_inputs(scope));
  auto channels = core::output_channels<true, 1>(scopeOutput.channels);
  auto cosineInput = core::block_inputs(cosine);
  auto cosineConnections = core::input_connections<true, 1, 1>(cosineInput.downstream);
  cosineConnections.connect(0, channels.at(0));
  cosineInput.downstream = cosineConnections.view();
  auto cosineOutput = core::bind_block(cosine, cosineInput);
  auto consumer = core::output_channels<false, 1>(cosineOutput.consumer);
  auto constantInput = core::block_inputs(constant);
  auto constantConnections = core::input_connections<true, 1, 1>(constantInput.downstream);
  constantConnections.connect(0, consumer.at(0));
  constantInput.downstream = constantConnections.view();
  core::bind_block(constant, constantInput);
  MockRuntime::start();
  if constexpr (std::is_same_v<T, f32>)
    CHECK_EQ(MockRuntime::lastF32(0, 0), 1);
  else
    CHECK_EQ(MockRuntime::lastF64(0, 0), 1);
  MockRuntime::close();
}

TEST_CASE("Scalar ports transport a custom struct through the same wiring API") {
  struct Message {
    i32  tag;
    Bool valid;
  };
  struct Inputs {
    Message message{};
  };
  struct Outputs {
    Message message{};
  };
  core::function<Outputs()>    source = [] { return Outputs{{42, true}}; };
  Message                      received{};
  core::function<void(Inputs)> sink = [&](Inputs input) { received = input.message; };
  auto                         output = core::bind_block(source, core::block_inputs(source));
  auto                         channels = core::output_channels<false, 1>(output.message);
  auto                         input = core::block_inputs(sink);
  auto                         connections = core::input_connections<false, 1, 1>(input.message);
  connections.connect(0, channels.at(0));
  input.message = connections.view();
  core::bind_block(sink, core::detail::move(input));
  CHECK_EQ(received.tag, 42);
  CHECK(received.valid);
}

TEST_CASE("Vector output views retain consumer storage and support direct arrays") {
  MockRuntime::reset();
  auto channels = core::output_channels<true, 2>(push::f_32::sinks::ScopeF32(0)().channels);
  (*channels.at(0))(3.f);
  (*channels.at(1))(5.f);
  CHECK_EQ(MockRuntime::lastF32(0, 0), 3.f);
  CHECK_EQ(MockRuntime::lastF32(0, 1), 5.f);
  i32  first = 7, second = 9;
  auto raw = core::output_channels<true, 2>(core::array<i32 *>{{&first, &second}});
  CHECK_EQ(*raw.at(0), 7);
  CHECK_EQ(*raw.at(1), 9);
  MockRuntime::close();
}

TEST_CASE("The metadata flag distinguishes scalar arrays from vectorized pointer lists") {
  core::array<i32> scalar{{7, 9}};
  auto             scalarOutput = core::output_channels<false, 1>(scalar);
  auto             scalarInput = core::input_connections<false, 1, 1>(scalar);
  scalarInput.connect(0, scalarOutput.at(0));
  auto value = scalarInput.view();
  REQUIRE_EQ(value.size(), 2);
  CHECK_EQ(value[1], 9);

  i32                first = 7, second = 9;
  core::array<i32 *> port;
  auto               connections = core::input_connections<true, 2, 1>(port);
  connections.connect(0, &first);
  connections.connect(0, &second);
  auto list = connections.view();
  REQUIRE_EQ(list.size(), 2);
  CHECK_EQ(list[0], &first);
  CHECK_EQ(list[1], &second);
  core::span<i32 *> mutablePort;
  auto              mutableConnections = core::input_connections<true, 1, 1>(mutablePort);
  mutableConnections.connect(0, &first);
  CHECK_EQ(mutableConnections.view()[0], &first);
}

TEST_CASE("Empty signatures, zero connections, and zero output channels share the contract") {
  u32                    calls = 0;
  core::function<void()> block = [&] { ++calls; };
  core::bind_block(block, core::block_inputs(block));
  CHECK_EQ(calls, 1);
  auto constant = push::f_32::sources::ConstF32(0);
  auto input = core::block_inputs(constant);
  auto connections = core::input_connections<true, 0, 0>(input.downstream);
  CHECK(connections.view().empty());
  auto scope = push::f_32::sinks::ScopeF32(1);
  auto output = core::bind_block(scope, core::block_inputs(scope));
  auto empty = core::output_channels<true, 0>(output.channels);
  (void)empty;
}

TEST_CASE("Grouped connections keep an empty channel between connected channels") {
  i32                                  first = 1;
  i32                                  third = 3;
  core::array<core::span<i32 *const>> port;
  auto                                 connections = core::input_connections<true, 1, 3>(port);
  connections.connect(0, &first);
  connections.connect(2, &third);
  auto groups = connections.view();
  REQUIRE_EQ(groups.size(), 3);
  REQUIRE_EQ(groups[0].size(), 1);
  CHECK(groups[1].empty());
  REQUIRE_EQ(groups[2].size(), 1);
  CHECK_EQ(*groups[0][0], 1);
  CHECK_EQ(groups[2][0], &third);
}

TEST_CASE("GPIO registration owns its configuration and applies the advertised channel limit") {
  MockRuntime::reset();
  core::array<u8> pins(10);
  for (u8 index = 0; index < pins.size(); ++index)
    pins[index] = index;
  auto gpio = push::f_32::sources::GpioInF32(42, 7, pins);
  pins[0] = 99;
  CHECK_EQ(MockRuntime::registeredGpioCount(), 1);
  REQUIRE_EQ(MockRuntime::registeredGpioPinCount(42), 8);
  CHECK_EQ(MockRuntime::registeredGpioPin(42, 0), 0);
  CHECK_EQ(MockRuntime::registeredGpioPin(42, 7), 7);
  (void)gpio;
}
