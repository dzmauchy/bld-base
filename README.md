
The project version is `VERSION` in `CMakeLists.txt`.

## Block interface

Every block uses input and output types, `I` and `O`, and implements or inherits
`O apply(I input)` (or `O apply()` when `I` is `void`). `I` is a struct or `void`.
`O` is a struct or `void` when there are no returned ports. Each field declares one
logical input or output port. A vectorized input port is `VectorizedInput<T>` (`std::vector<T*>`),
and a vectorized output port is a callable `VectorizedOutput<T>` which accepts a `u8` channel
count and returns `std::vector<T*>`. `Block<I, O>` exposes these types as `Input` and
`Output` and supports virtual dispatch through the same interface.

Use `std::vector<T>(count)` for value-initialized elements,
`std::vector<T>(count, value)` for repeated values, or `std::vector<T>(first, last)`
to copy an existing buffer. `std::vector<i32>{3}` contains one value, while
`std::vector<i32>(3)` contains three zero-initialized values.
Vectors can grow and resize; blocks configure channel storage before returning
consumer pointers. Rebuilding or reallocating that storage invalidates its
previous consumer pointers, so configure channels before wiring consumers or
starting the runtime.

`VectorizedOutput<T>` is an alias for `std::function<std::vector<T*>(u8)>` and owns its callable.
`VectorizedOutput<T>{}` creates an empty output; calling it throws `std::bad_function_call`.
Assign a function, functor, or capturing lambda directly to bind an output.
Member bindings use lambdas such as `[this](u8 count) { return makeChannels(count); }`.
Objects captured by pointer or reference must remain alive at the same address
while the output is used. Storing the callable may allocate memory.

The push blocks retain their stream behavior: `apply` wires consumer streams,
and the runtime delivers values through callbacks. Constructor arguments
configure the blocks. All input and output wrappers live in `base/ports.hpp`.
Each f32 and f64 block with outputs has its own `O` struct in that file,
with documentation on every output field:

| Block | Input fields | Output type `O` | Output fields |
| --- | --- | --- | --- |
| Constant and generators | `downstream` | `void` | None |
| Cosine / sine transformers | `downstream` | `CosF32Output` / `SinF32Output` | `consumer` |
| Sum / product | `downstream` | `SumF32Output` / `ProductF32Output` | `channels` |
| Scope | None (`void`) | `ScopeF32Output` | `channels` |
| GPIO input | `pins` (consumer groups in configured pin order) | `void` | None |

The f64 blocks have corresponding `F64Output` structs. `apply` returns the
complete output struct by value; callers access individual outputs through its
documented fields. Blocks with multiple outputs declare one field per output
and populate those fields in `apply`.

```cpp
#include <base/f32_blocks.hpp>

push::f_32::sinks::ScopeF32 scope(0);
push::f_32::transformers::CosF32 cosine(1);
push::f_32::sources::ConstF32 constant(2, 0.f);

void wire() {
  push::f_32::sinks::ScopeF32Output scopeOutput = scope.apply();
  push::f_32::transformers::CosF32Output cosineOutput = cosine.apply({.downstream = scopeOutput.channels(1)});
  constant.apply({.downstream = {cosineOutput.consumer}});
}
```

Call `wire()` before starting the runtime. Blocks must remain alive at the same
address while their consumers and runtime callbacks are in use. All blocks in
`push::f_32` and `push::f_64` are concrete classes or aliases, such as `CosF32` and
`ScopeF32`, and are named without template arguments. For custom ports, use the
generic implementation templates such as `push::Scope<O>`. Custom port structs
must supply the same fields and a `Value` type alias where the default struct has one.

Documentation uses this format on each declaration:

```cpp
/**
 * Downstream
 * @brief Streams that receive values emitted by the block.
 * @image consumer.svg
 */
VectorizedInput<Consumer<T>> downstream{};
```

## Building

Requires Clang 23, CMake 3.24 or newer, and a matching libstdc++ (GCC 14 on Ubuntu 24.04, because Clang 23 links with that toolchain).

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++-23
cmake --build build
```

## Testing

```sh
ctest --test-dir build --output-on-failure
```

## Headers

`cmake --install build --prefix dist` installs the public headers under `dist/include`. Other block libraries include the shared headers from that prefix:

```cpp
#include <core/block.hpp>
#include <core/callback.hpp>
#include <core/hal.hpp>
#include <core/math/trig.hpp>
```

Sources live under `src/`. Install and the release archive use the contents of that directory as the include prefix:

- `core/` — shared HAL and runtime headers (`Block`, `Callback`, `VectorizedInput`, `VectorizedOutput`, `Maybe`, `move()`, member adapters), included as `#include <core/....hpp>`
- `core/math/` — `wrapTwoPi` and trigonometry helpers, included as `#include <core/math/trig.hpp>`
- `base/` — this library's push blocks; include `base/f32_blocks.hpp` or `base/f64_blocks.hpp` for the corresponding endpoints

The versioned release archive contains `base/` and `core/`, including `core/math/`,
plus `meta.json` at the archive root. The release workflow generates the metadata before packaging.

## Metadata

Requires Node.js 24 or newer and `clang.js`, `clang.wasm`, and `sysroot.tgz`
from [clang-wasm clang-23.1.2](https://github.com/dzmauchy/clang-wasm/releases/tag/clang-23.1.2).
Put the three assets in `.cache/clang-23.1.2/`, or pass their directory:

```sh
npm run build:meta -- /path/to/clang-assets
```

The script installs the archive directly into clang's in-memory filesystem and
parses every `src/**/*.hpp` using the JSON AST and documentation comments.
It writes `.cache/meta.json`, creating `.cache/` in the project root if needed,
regardless of the working directory.
No npm dependencies or native compiler are needed. The release's browser-only
JS glue runs in an isolated Node worker. Its `unsupported syscall: __syscall_prlimit64`
warning is harmless for AST generation.

The output has `namespaces` and `blocks` arrays. Namespace entries use fully
qualified IDs such as `push::f_32::sinks` and contain `id`, `name`, `description`,
and `icon`. Block and port entries also contain `namespace`; blocks have `inputs`,
`outputs`, and `parameters` arrays. Each parameter object contains `id`, `namespace`,
`name`, `description`, `icon`, and a `control` object describing the UI control
(e.g., `type`, `min`, `max`, `step`). Constructor parameters are documented in
comments on the constructor using nested `@param <id> <Name>` followed by description text,
`@icon <icon.svg>`, `@control <type>`, `@min`, `@max`, and `@step`, excluding `blockId`. Names come from
the first documentation paragraph, descriptions from `@brief`/`@details`, and icons
from `@image`. Missing documentation falls back to the declaration name and empty
description/icon strings. `namespace` is the enclosing C++ scope (`""` for global
scope). Port scopes are those of their declaring structs. IDs are unqualified
declaration names; port IDs are field names. C++ type expressions and
compiler-generated IDs are omitted.

Blocks are descendants of `Block` whose `I` template parameter has a default and
whose `O` parameter is either absent or has a default, plus concrete classes and
aliases that provide those arguments in a base or aliased specialization.
A missing `O` means `void`. Ports come from the default
or explicitly supplied structs; `void` produces an empty output array. Generic
implementation templates without the required defaults are omitted from `blocks`.
Repeated namespace declarations and implicit template instances are deduplicated.

Run the metadata checks with the same assets available:

```sh
npm test
```

For IDE support and strict TypeScript checks, install the development dependencies:

```sh
npm ci
npm run typecheck
```

`npm test` runs the metadata checks, and `npm run build:meta` generates metadata.
The scripts still run directly with Node.js without a compilation step.

## Host bindings

The host supplies the `extern "C"` functions declared in `core/hal.hpp` directly.
Firmware links its definitions into the application; a browser runtime supplies
the corresponding WebAssembly imports. The host owns lifecycle callbacks,
timers, GPIO, ADC/DAC, observations, time in milliseconds, random values, and math.
Only functions used by the application need bindings.

Include `core/hal.hpp` and supply the functions, such as `send_value_f32`,
`read_gpio`, and `set_interval`. This is the same contract for every host.

Registration functions receive a `Callback*` owned by a block. The host retains
that pointer until the registration ends and invokes `(*callback)()` when the
event occurs. For a browser host, the application must expose a WebAssembly
callback entry point that performs this C++ call; JavaScript treats the pointer
as an opaque value. Blocks must stay alive while callbacks are registered.

The Release headers workflow publishes those headers as a GitHub Release tagged with the project version (`v0.1.0`). Run it manually from the Actions tab. Running it again for the same version replaces that release so the tag matches the selected commit. A new version in `CMakeLists.txt` publishes a new tag and leaves the previous release in place.
