#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>
#include <core/diagram.hpp>

namespace {
using Callback = core::function<void()>;
Callback *starts[16]{};
Callback *closes[16]{};
Callback *timers[16]{};
Callback *gpio = nullptr;
u32       startCount = 0, closeCount = 0, timerCount = 0;
u64       now = 0;
f32       observed32[8]{};
f64       observed64[8]{};
#ifdef TEST_GPIO_REGISTRATION
u32             registeredBlock = 0;
u16             registeredPort = 0;
core::array<u8> registeredPins;
#endif
} // namespace

extern "C" {
void on_start(Callback *callback) { starts[startCount++] = callback; }
void on_close(Callback *callback) { closes[closeCount++] = callback; }
u32  set_interval(u32,
                  Callback *callback) {
  timers[timerCount] = callback;
  return ++timerCount;
}
void clear_interval(u32 id) { timers[id - 1] = nullptr; }
bool read_gpio(u32,
               u8) {
  return true;
}
u32 set_gpio(u32,
             u8,
             Callback *callback) {
  gpio = callback;
  return 1;
}
void clear_gpio(u32) { gpio = nullptr; }
#ifdef TEST_GPIO_REGISTRATION
void register_gpio_block(u32                    blockId,
                         u16                    port,
                         const core::array<u8> &pins) {
  registeredBlock = blockId;
  registeredPort = port;
  registeredPins = pins;
}
#endif
void send_value_f32(u32,
                    u8  channel,
                    f32 value) {
  observed32[channel] = value;
}
void send_value_f64(u32,
                    u8  channel,
                    f64 value) {
  observed64[channel] = value;
}
u64 get_time() { return now; }
f32 random_f32() { return 0.5f; }
f64 random_f64() { return 0.5; }
}

#define CHECK(expression)                                                                          \
  do {                                                                                             \
    if (!(expression))                                                                             \
      return __LINE__;                                                                             \
  } while (false)

extern "C" int block_checks() {
  startCount = closeCount = timerCount = 0;
  now = 0;
  auto scope32 = push::f_32::sinks::ScopeF32(0);
  auto scope64 = push::f_64::sinks::ScopeF64(1);
  auto channels32 = scope32().channels(4);
  auto channels64 = scope64().channels(4);
  auto sine = push::f_32::sources::SinGenF32(2, 10, 1, 2);
  auto cosine = push::f_64::sources::CosGenF64(3, 10, 1, 3);
  auto sum = push::f_32::transformers::SumF32(4, 10);
  auto product = push::f_64::transformers::ProductF64(5, 10);
  auto sumOut = sum({.downstream = channels32.first(1)});
  auto productOut = product({.downstream = channels64.first(1)});
  auto terms = sumOut.channels(2);
  auto factors = productOut.channels(2);
  auto constant = push::f_32::sources::ConstF32(6, 3);
  constant({.downstream = terms.first(1)});
  // A copy must retain the same state and the host's borrowed callback pointers.
  auto constantCopy = constant;
  constant = nullptr;
  core::array<core::function<void(f32)> *> sineSinks{{channels32[1]}};
  core::array<core::function<void(f64)> *> cosineSinks{{channels64[1]}};
  sine({.downstream = sineSinks});
  cosine({.downstream = cosineSinks});
  auto input = push::f_32::sources::GpioInF32(7, 0);
  core::array<VectorizedInput<core::function<void(f32)>>> pinGroups{{channels32.first(1)}};
  input({.pins = pinGroups});
  auto                                     transform = push::f_64::transformers::SinF64(8);
  core::array<core::function<void(f64)> *> transformSinks{{channels64[2]}};
  auto receive = transform({.downstream = transformSinks}).consumer;
  (*receive)(core::pi_v<f64> / 2);
  CHECK(observed64[2] > 0.999999 && observed64[2] < 1.000001);
  (*terms[1])(4);
  (*factors[0])(2);
  (*factors[1])(5);
  for (u32 i = 0; i < startCount; ++i)
    (*starts[i])();
  now = 250;
  for (u32 i = 0; i < timerCount; ++i)
    (*timers[i])();
  CHECK(observed32[0] == 7);
  CHECK(observed64[0] == 10);
  CHECK(observed32[1] > 1.99999f && observed32[1] < 2.00001f);
  CHECK(observed64[1] > -0.000001 && observed64[1] < 0.000001);
  (*gpio)();
  CHECK(observed32[0] == 1);
  for (u32 i = 0; i < closeCount; ++i)
    (*closes[i])();
  for (u32 i = 0; i < timerCount; ++i)
    CHECK(timers[i] == nullptr);
  CHECK(gpio == nullptr);
  return 0;
}

extern "C" int diagram_checks() {
  startCount = closeCount = timerCount = 0;
  auto scope32 = push::f_32::sinks::ScopeF32(0);
  auto scope64 = push::f_64::sinks::ScopeF64(1);
  auto out32 = core::bind_block(scope32, core::block_inputs(scope32));
  auto out64 = core::bind_block(scope64, core::block_inputs(scope64));
  auto channels32 = core::output_channels<true, 4>(out32.channels);
  auto channels64 = core::output_channels<true, 2>(out64.channels);
  auto constant32 =
      push::f_32::sources::ConstF32(2, core::config_arg<0>(push::f_32::sources::ConstF32, 3.5));
  auto constant64 =
      push::f_64::sources::ConstF64(3, core::config_arg<0>(push::f_64::sources::ConstF64, 7.25));
  auto input =
      push::f_32::sources::GpioInF32(4, core::config_arg<0>(push::f_32::sources::GpioInF32, 7),
                                     core::config_arg<1>(push::f_32::sources::GpioInF32, 0, 1));
#ifdef TEST_GPIO_REGISTRATION
  CHECK(registeredBlock == 4 && registeredPort == 7);
  CHECK(registeredPins.size() == 2 && registeredPins[0] == 0 && registeredPins[1] == 1);
#endif
  {
    auto in32 = core::block_inputs(constant32);
    auto in64 = core::block_inputs(constant64);
    auto gpioIn = core::block_inputs(input);
    auto connections32 = core::input_connections<true, 2, 2>(in32.downstream);
    auto connections64 = core::input_connections<true, 2, 2>(in64.downstream);
    auto groups = core::input_connections<true, 2, 2>(gpioIn.pins);
    connections32.connect(0, channels32.at(0));
    connections32.connect(1, channels32.at(1));
    connections64.connect(0, channels64.at(0));
    connections64.connect(1, channels64.at(1));
    groups.connect(1, channels32.at(2));
    groups.connect(1, channels32.at(3));
    in32.downstream = connections32.view();
    in64.downstream = connections64.view();
    gpioIn.pins = groups.view();
    core::bind_block(constant32, in32);
    core::bind_block(constant64, in64);
    core::bind_block(input, gpioIn);
  }
  for (u32 i = 0; i < startCount; ++i)
    (*starts[i])();
  CHECK(observed32[0] == 3.5f && observed32[1] == 3.5f);
  CHECK(observed64[0] == 7.25 && observed64[1] == 7.25);
  (*gpio)();
  CHECK(observed32[2] == 1 && observed32[3] == 1);
  for (u32 i = 0; i < closeCount; ++i)
    (*closes[i])();
  CHECK(gpio == nullptr);
  return 0;
}

extern "C" void diagram_scalar_index() {
  i32  port = 0;
  auto connections = core::input_connections<false, 1, 0>(port);
  connections.connect(0, 1);
}
extern "C" void diagram_connection_overflow() {
  i32                    value = 0;
  core::span<i32 *const> port;
  auto                   connections = core::input_connections<true, 1, 1>(port);
  connections.connect(0, &value);
  connections.connect(0, &value);
}
extern "C" void diagram_group_index() {
  i32                                 value = 0;
  core::array<core::span<i32 *const>> port;
  auto                                connections = core::input_connections<true, 1, 1>(port);
  connections.connect(1, &value);
}
extern "C" void diagram_output_width() {
  auto channels = core::output_channels<true, 2>(core::array<i32 *>{});
  (void)channels;
}
extern "C" void diagram_gpio_width() {
  auto gpioBlock = push::f_32::sources::GpioInF32(0);
  auto input = core::block_inputs(gpioBlock);
  input.pins = decltype(input.pins)(2);
  core::bind_block(gpioBlock, input);
}
