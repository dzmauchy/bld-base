#pragma once

/**
 * <type name="Boolean" description="A boolean value that can be either true or false"/>
 */
using Bool = bool;

/**
 * <type name="Signed 8-bit Integer" description="A signed integer value that can hold values from -128 to 127"/>
 */
using i8 = signed char;
/**
 * <type name="Unsigned 8-bit Integer" description="An unsigned integer value that can hold values from 0 to 255"/>
 */
using u8 = unsigned char;
/**
 * <type name="Signed 16-bit Integer" description="A signed integer value that can hold values from -32768 to 32767"/>
 */
using i16 = short;
/**
 * <type name="Unsigned 16-bit Integer" description="An unsigned integer value that can hold values from 0 to 65535"/>
 */
using u16 = unsigned short;
/**
 * <type name="Signed 32-bit Integer" description="A signed integer value that can hold values from -2147483648 to 2147483647"/>
 */
using i32 = int;
/**
 * <type name="Unsigned 32-bit Integer" description="An unsigned integer value that can hold values from 0 to 4294967295"/>
 */
using u32 = unsigned int;
/**
 * <type name="Signed 64-bit Integer" description="A signed integer value that can hold values from -9223372036854775808 to 9223372036854775807"/>
 */
using i64 = long long;
/**
 * <type name="Unsigned 64-bit Integer" description="An unsigned integer value that can hold values from 0 to 18446744073709551615"/>
 */
using u64 = unsigned long long;
/**
 * <type name="32-bit Floating Point Number" description="A floating point value that can hold values from approximately -3.40282347e+38 to 3.40282347e+38"/>
 */
using f32 = float;
/**
 * <type name="64-bit Floating Point Number" description="A floating point value that can hold values from approximately -1.7976931348623157e+308 to 1.7976931348623157e+308"/>
 */
using f64 = double;

/**
 * <type icon="consumer.svg" name="Consumer" description="A stream of data that can be pushed to">
 *   <arg name="T" description="Consumed value type"/>
 * </type>
 */
template <typename... Args>
class Consumer {
 public:
  virtual ~Consumer() = default;
  virtual void operator()(Args... args) = 0;
};
