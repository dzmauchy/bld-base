#pragma once

using Bool = bool;

using i8 = signed char;
using u8 = unsigned char;
using i16 = short;
using u16 = unsigned short;
using i32 = int;
using u32 = unsigned int;
using i64 = long long;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;

/**
 * Consumer
 * @brief A stream of data that can be pushed to
 * @image consumer.svg
 */
template <typename... Args> class Consumer {
public:
  virtual ~Consumer() = default;
  virtual void operator()(Args... args) = 0;
};
