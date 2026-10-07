The project version is `VERSION` in `CMakeLists.txt`.

## Block interface

Each block is a factory function accepting configuration parameters and returning
`std::function<O(I)>`. Blocks with no inputs return `std::function<O()>`.
`I` and `O` are structs or `void`; each field declares one logical port. Calling the returned function wires inputs and
returns the complete
output struct by value. Runtime callbacks deliver stream values afterward.
A vectorized input is `VectorizedInput<T>` (`std::span<T* const>`); a vectorized output
is `VectorizedOutput<T>`, an alias for `std::function<std::span<T *const>(u8)>` that
accepts a channel count and returns a borrowed view of consumer pointers.
The pointer entries are read-only; the consumers remain mutable. Inputs accept
`std::vector`, `std::array`, and C-style pointer arrays. Built-in blocks copy input
pointer lists when wired, so the lists only need to survive the wiring call.

Use `std::vector<T>(count)` for value-initialized elements,
`std::vector<T>(count, value)` for repeated values, or `std::vector<T>(first, last)`
to copy an existing buffer. `std::vector<i32>{3}` contains one value, while
`std::vector<i32>(3)` contains three zero-initialized values.
Vectors can grow and resize; blocks configure channel storage before returning
consumer pointers. Rebuilding or reallocating that storage invalidates its
previous spans and consumer pointers, so configure channels before wiring consumers or
starting the runtime.

`VectorizedOutput<T>` aliases `std::function<std::span<T *const>(u8)>` and owns its callable.
Custom outputs must return spans over storage that outlives their use, such as
arrays or vectors owned by the callable; returning a view of a local vector dangles.
An empty `std::function` throws `std::bad_function_call` when invoked.
Assign a function, functor, or capturing lambda directly to bind an output.
Bind captured state with a lambda such as `[channels](u8 count) { /* build consumers */ }`.
Objects captured by pointer or reference must remain alive at the same address
while the output is used. Storing the callable may allocate memory.

Factory arguments configure the blocks; invoking the returned callable wires
consumer streams. All input and output wrappers live in `base/ports.hpp`.
Each f32 and f64 block with outputs has its own `O` struct in that file,
with documentation on every output field:

| Block                      | Input fields                                     | Output type `O`                     | Output fields |
|----------------------------|--------------------------------------------------|-------------------------------------|---------------|
| Constant and generators    | `downstream`                                     | `void`                              | None          |
| Cosine / sine transformers | `downstream`                                     | `CosF32Output` / `SinF32Output`     | `consumer`    |
| Sum / product              | `downstream`                                     | `SumF32Output` / `ProductF32Output` | `channels`    |
| Scope                      | None (`void`)                                    | `ScopeF32Output`                    | `channels`    |
| GPIO input                 | `pins` (consumer groups in configured pin order) | `void`                              | None          |

The f64 blocks have corresponding `F64Output` structs. Callers access individual
outputs through their documented fields. Blocks with multiple outputs declare
one field per output.

```cpp
#include <base/f32_blocks.hpp>
#include <array>

auto scope = push::f_32::sinks::ScopeF32(0);
auto cosine = push::f_32::transformers::CosF32(1);
auto constant = push::f_32::sources::ConstF32(2, 0.f);

void wire() {
  push::f_32::sinks::ScopeF32Output scopeOutput = scope();
  push::f_32::transformers::CosF32Output cosineOutput = cosine({.downstream = scopeOutput.channels(1)});
  constant({.downstream = std::array{cosineOutput.consumer}});
}
```

Call `wire()` before starting the runtime. Blocks are composed from functions and
capturing lambdas. Configuration is captured by value; mutable stream lists,
channel values, and callback storage use shared standard containers. There are
no custom block/state classes, virtual methods, or member callback adapters.
Functions can be moved, copied, or stored in growing vectors after wiring.
Copies share the same logical block; call the factory again for an independent block.

Stream consumers use `std::function<void(T)>`, and lifecycle callbacks use
`std::function<void()>`. Assign lambdas or functions directly:

```cpp
std::function<void(f32)> receive = [](f32 value) { /* process value */ };
auto source = push::f_32::sources::ConstF32(0, 3.f);
source({.downstream = std::array{&receive}});
```

Raw consumer pointers and host registrations borrow their callables. Keep an owner
alive until those pointers are unused and the runtime has closed and released its
callbacks. A vectorized output owns its captured channel storage, so retaining the
output also retains its consumers and the pointer list behind its returned spans.
Keep that output callable alive while using those spans. Scalar consumer pointers require the producing
block callable to stay alive. Rebuilding channels invalidates previous spans and pointers.
Rewiring updates shared stream lists; wire each source once before runtime start.
Shared factory functions live in `push::detail` and call the HAL directly.
For custom blocks, return `std::function<O(I)>` from a factory using
lambdas and your input/output structs.

Documentation uses this format on each declaration:

```cpp
/**
 * Downstream
 * @brief Streams that receive values emitted by the block.
 * @image consumer.svg
 */
VectorizedInput<std::function<void(T)>> downstream{};
```

## Building

Requires Clang 23, CMake 3.24 or newer, and a matching libstdc++ (GCC 14 on Ubuntu 24.04, because Clang 23 links with
that toolchain).

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

`cmake --install build --prefix dist` installs the public headers under `dist/include`. Other block libraries include
the shared headers from that prefix:

```cpp
#include <functional>
#include <core/types.hpp>
#include <core/hal.hpp>
```

Sources live under `src/`. Install and the release archive use the contents of that directory as the include prefix:

- `core/` — shared HAL and runtime headers (scalar types, `VectorizedInput`, and `VectorizedOutput`), included as
  `#include <core/....hpp>`
- `base/` — this library's push blocks; include `base/f32_blocks.hpp` or `base/f64_blocks.hpp` for the corresponding
  endpoints

The versioned release archive contains `base/` and `core/`,
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
`outputs`, and `parameters` arrays. Vectorized input and output ports include
`"vectorized": true` when their fields use `VectorizedInput` or `VectorizedOutput`;
scalar ports omit that property. Detection follows port
types and their aliases, including grouped inputs and equivalent standard container types.
Each parameter object contains `id`, `namespace`,
`name`, `description`, `icon`, and a `control` object describing the UI control (e.g., `type`, `min`, `max`, `step`).
Factory parameters are documented in
comments on the function using nested `@param <id> <Name>` followed by description text,
`@icon <icon.svg>`, `@control <type>`, `@min`, `@max`, and `@step`, excluding `blockId`. Names come from
the first documentation paragraph, descriptions from `@brief`/`@details`, and icons
from `@image`. Missing documentation falls back to the declaration name and empty
description/icon strings. `namespace` is the enclosing C++ scope (`""` for global
scope). Port scopes are those of their declaring structs. IDs are unqualified
declaration names; port IDs are field names. C++ type expressions and
compiler-generated IDs are omitted.

Blocks are factory functions returning `std::function<O(I)>` or
`std::function<O()>`, with input/output structs or `void`. Metadata discovery also
follows named aliases and trailing return syntax. Other callable signatures are
skipped. Ports come from the specified structs; `void` produces an empty port array. Helpers and declarations
inside `detail` namespaces are omitted. Repeated namespace declarations and implicit
template instances are deduplicated.

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

## Freestanding containers

`core/lib.hpp` provides `core::function<R(Args...)>` and a fixed-size, contiguous
`core::array<T>` for freestanding applications. On Wasm it includes `wasm.hpp`
and uses clang-wasm's TLSF-backed new/delete, including aligned allocation.
It needs no libc++, RTTI, or exception support. Native builds use `<new>`.
The existing block headers still use the standard library.

Functions own copyable lambdas, functors, or function pointers. Copies duplicate
captured values; moves transfer ownership and empty the source. An empty invocation
traps. An array's size is chosen at construction, with no `N` template parameter
and no growth or resize operations. Arrays support count/value construction,
C arrays, pointer ranges, copy/move, indexing, iteration, and fill. Copies own
separate storage; moves transfer the buffer and empty the source. Assignment
replaces the owned buffer. Elements are destroyed when their buffer is released.
`at()` checks bounds and traps on invalid indices; `operator[]` requires a valid
index. Size overflow and Wasm allocation failure also trap.

```cpp
#include <core/lib.hpp>

core::function<int(int)> twice = [](int value) { return value * 2; };
core::array<int> zeros(3);              // Three zero-initialized values.
core::array<int> repeated(3, 7);        // Three sevens.
core::array<int> values{{1, 2, 3}};     // Copy a C array of values.
values[0] = twice(values[0]);
```

Native tests cover these types along with the blocks. Run the freestanding
compile/link/runtime checks against a built clang-wasm sysroot with:

```sh
node scripts/test-core-lib.mjs ../clang-wasm/dist/sysroot clang++
```

This links only compiler-rt (which bundles the TLSF runtime), checks ownership
and alignment repeatedly, and verifies traps and the absence of host imports.

## Host bindings

The host supplies the `extern "C"` functions declared in `core/hal.hpp` directly.
Firmware links its definitions into the application; a browser runtime supplies
the corresponding WebAssembly imports. The host owns lifecycle callbacks,
timers, GPIO, ADC/DAC, observations, time in milliseconds, and random values.
Blocks use standard C++ math functions such as `std::sin`, `std::cos`, and `std::isfinite`.
Unset aggregate channels use `std::numeric_limits<T>::quiet_NaN()`.
Generator phase wrapping uses `std::fmod` with `std::numbers::pi_v`.
Only functions used by the application need bindings.

Include `core/hal.hpp` and supply the functions, such as `send_value_f32`,
`read_gpio`, and `set_interval`. This is the same contract for every host.

Registration functions receive a `std::function<void()>*` pointing to an owned callable. The host retains
that pointer until the registration ends and invokes `(*callback)()` when the
event occurs. For a browser host, the application must expose a WebAssembly
callback entry point that performs this C++ call; JavaScript treats the pointer
as an opaque value. Keep an owning block callable alive while callbacks are registered. Hosts must
compile against these headers and invoke the callable directly; callbacks are invoked through their `std::function`
interface.

The Release headers workflow publishes those headers as a GitHub Release tagged with the project version (`v0.1.0`). Run
it manually from the Actions tab. Running it again for the same version replaces that release so the tag matches the
selected commit. A new version in `CMakeLists.txt` publishes a new tag and leaves the previous release in place.
