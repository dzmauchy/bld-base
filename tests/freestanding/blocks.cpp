#include <base/f32_blocks.hpp>
#include <base/f64_blocks.hpp>

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
