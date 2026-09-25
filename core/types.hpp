#pragma once

#define BLD_C 0

/*{"kind":"type","id":"bool","name":"Boolean","description":"A boolean value that can be either true or false","as_arg_compatible_with":["i8","u8","i16","u16","i32","u32","i64","u64","f32","f64"]}*/
using Bool = bool;

/*{"kind":"type","id":"i8","name":"Signed 8-bit Integer","description":"A signed integer value that can hold values from -128 to 127","as_arg_compatible_with":["bool"]}*/
using i8 = signed char;
/*{"kind":"type","id":"u8","name":"Unsigned 8-bit Integer","description":"An unsigned integer value that can hold values from 0 to 255","as_arg_compatible_with":["bool"]}*/
using u8 = unsigned char;
/*{"kind":"type","id":"i16","name":"Signed 16-bit Integer","description":"A signed integer value that can hold values from -32768 to 32767","as_arg_compatible_with":["bool","i8","u8"]}*/
using i16 = short;
/*{"kind":"type","id":"u16","name":"Unsigned 16-bit Integer","description":"An unsigned integer value that can hold values from 0 to 65535","as_arg_compatible_with":["bool","i8","u8"]}*/
using u16 = unsigned short;
/*{"kind":"type","id":"i32","name":"Signed 32-bit Integer","description":"A signed integer value that can hold values from -2147483648 to 2147483647","as_arg_compatible_with":["bool","i8","u8","i16","u16"]}*/
using i32 = int;
/*{"kind":"type","id":"u32","name":"Unsigned 32-bit Integer","description":"An unsigned integer value that can hold values from 0 to 4294967295","as_arg_compatible_with":["bool","i8","u8","i16","u16"]}*/
using u32 = unsigned int;
/*{"kind":"type","id":"i64","name":"Signed 64-bit Integer","description":"A signed integer value that can hold values from -9223372036854775808 to 9223372036854775807","as_arg_compatible_with":["bool","i8","u8","i16","u16","i32","u32"]}*/
using i64 = long long;
/*{"kind":"type","id":"u64","name":"Unsigned 64-bit Integer","description":"An unsigned integer value that can hold values from 0 to 18446744073709551615","as_arg_compatible_with":["bool","i8","u8","i16","u16","i32","u32"]}*/
using u64 = unsigned long long;
/*{"kind":"type","id":"f32","name":"32-bit Floating Point Number","description":"A floating point value that can hold values from approximately -3.40282347e+38 to 3.40282347e+38","as_arg_compatible_with":["bool","i8","u8","i16","u16","i32","u32","i64","u64"]}*/
using f32 = float;
/*{"kind":"type","id":"f64","name":"64-bit Floating Point Number","description":"A floating point value that can hold values from approximately -1.7976931348623157e+308 to 1.7976931348623157e+308","as_arg_compatible_with":["bool","i8","u8","i16","u16","i32","u32","i64","u64"]}*/
using f64 = double;
