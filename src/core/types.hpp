#pragma once

/**
 * Boolean
 * @brief A boolean value that can be either true or false
 * @image type.svg
 */
using Bool = bool;

/**
 * Signed 8-bit Integer
 * @brief A signed integer value that can hold values from -128 to 127
 * @image type.svg
 */
using i8 = signed char;
/**
 * Unsigned 8-bit Integer
 * @brief An unsigned integer value that can hold values from 0 to 255
 * @image type.svg
 */
using u8 = unsigned char;
/**
 * Signed 16-bit Integer
 * @brief A signed integer value that can hold values from -32768 to 32767
 * @image type.svg
 */
using i16 = short;
/**
 * Unsigned 16-bit Integer
 * @brief An unsigned integer value that can hold values from 0 to 65535
 * @image type.svg
 */
using u16 = unsigned short;
/**
 * Signed 32-bit Integer
 * @brief A signed integer value that can hold values from -2147483648 to 2147483647
 * @image type.svg
 */
using i32 = int;
/**
 * Unsigned 32-bit Integer
 * @brief An unsigned integer value that can hold values from 0 to 4294967295
 * @image type.svg
 */
using u32 = unsigned int;
/**
 * Signed 64-bit Integer
 * @brief A signed integer value that can hold values from -9223372036854775808 to 9223372036854775807
 * @image type.svg
 */
using i64 = long long;
/**
 * Unsigned 64-bit Integer
 * @brief An unsigned integer value that can hold values from 0 to 18446744073709551615
 * @image type.svg
 */
using u64 = unsigned long long;
/**
 * 32-bit Floating Point Number
 * @brief A floating point value that can hold values from approximately -3.40282347e+38 to 3.40282347e+38
 * @image type.svg
 */
using f32 = float;
/**
 * 64-bit Floating Point Number
 * @brief A floating point value that can hold values from approximately -1.7976931348623157e+308 to 1.7976931348623157e+308
 * @image type.svg
 */
using f64 = double;

/**
 * Consumer
 * @brief A stream of data that can be pushed to
 * @image consumer.svg
 */
template <typename... Args>
class Consumer {
 public:
  virtual ~Consumer() = default;
  virtual void operator()(Args... args) = 0;
};
