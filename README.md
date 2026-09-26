# BASE block library (C++)

A header-only C++ library. Namespaces, types, block classes, and input/output fields carry documentation comments that core can read from the clang AST. Production sources are headers; the CMake target builds those headers and the native tests.

The project version is `VERSION` in `CMakeLists.txt`.

## Block interface

Every block has two template parameters, `I` and `O`, and implements or inherits
`O apply(I input)`. `I` is a struct. `O` is a struct or `void` when there are no
returned ports. Each field declares one logical input or output port. A vectorized
output is one field, regardless of how many elements it contains. `Block<I, O>` exposes these types as `Input` and
`Output` and supports virtual dispatch through the same interface.

The push blocks retain their stream behavior: `apply` wires consumer streams,
and the runtime delivers values through callbacks. Constructor arguments still
configure the blocks. All input and output wrappers live in `base/ports.hpp`.
Each f32 and f64 block with outputs has its own default `O` struct in that file,
with documentation on every output field:

| Block | Input fields | Output type `O` | Output fields |
| --- | --- | --- | --- |
| Constant and generators | `downstream` | `void` | None |
| Cosine / sine transformers | `downstream` | `CosF32Output` / `SinF32Output` | `consumer` |
| Sum / product | `downstream`, `channelCount` | `SumF32Output` / `ProductF32Output` | `channels` |
| Scope | `channelCount` | `ScopeF32Output` | `channels` |
| GPIO input | `pins` (consumer groups in configured pin order) | `void` | None |

The f64 blocks have corresponding `F64Output` structs. `apply` returns the
complete output struct by value; callers access individual outputs through its
documented fields. Blocks with multiple outputs declare one field per output
and populate those fields in `apply`.

```cpp
#include <base/f32_blocks.hpp>

push::f32::sinks::ScopeF32<> scope(0);
push::f32::transformers::CosF32<> cosine(1);
push::f32::sources::ConstF32<> constant(2, 0.f);

void wire() {
  push::f32::sinks::ScopeF32Output scopeOutput = scope.apply({.channelCount = 1});
  push::f32::transformers::CosF32Output cosineOutput = cosine.apply({.downstream = scopeOutput.channels});
  constant.apply({.downstream = {cosineOutput.consumer}});
}
```

Call `wire()` before starting the runtime. Blocks must remain alive at the same
address while their consumers and runtime callbacks are in use. Use `Class<>`
when naming a default block specialization in a container or function signature.
Custom port structs for the provided push implementations must supply the same
fields and a `Value` type alias where the default struct has one.

Documentation uses this format on each declaration:

```cpp
/**
 * Downstream
 * @brief Streams that receive values emitted by the block.
 * @image consumer.svg
 */
Vectorized<Consumer<T>> downstream{};
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

- `core/` — shared HAL and runtime headers (`Block`, `Callback`, `Array`, `Maybe`, `move()`, member adapters), included as `#include <core/....hpp>`
- `core/math/` — `wrapTwoPi` and trigonometry helpers, included as `#include <core/math/trig.hpp>`
- `base/` — this library's push blocks; include `base/f32_blocks.hpp` or `base/f64_blocks.hpp` for the corresponding endpoints

The versioned release archive contains `base/` and `core/`, including `core/math/`,
plus `meta.json` at the archive root. The release workflow generates the metadata before packaging.

## Metadata

Requires Node.js 24 or newer and `clang.js`, `clang.wasm`, and `sysroot.tgz`
from [clang-wasm clang-23.1.2](https://github.com/dzmauchy/clang-wasm/releases/tag/clang-23.1.2).
Put the three assets in `.cache/clang-23.1.2/`, or pass their directory:

```sh
node .github/scripts/generate-meta.mjs /path/to/clang-assets
```

The script installs the archive directly into clang's in-memory filesystem and
parses every `src/**/*.hpp` using the JSON AST and documentation comments.
It writes `.cache/meta.json`, creating `.cache/` in the project root if needed,
regardless of the working directory.
No npm dependencies or native compiler are needed. The release's browser-only
JS glue runs in an isolated Node worker. Its `unsupported syscall: __syscall_prlimit64`
warning is harmless for AST generation.

The output has `namespaces`, `types`, and `blocks` arrays. Namespace entries use
fully qualified IDs such as `push::f32::sinks` and contain `id`, `name`,
`description`, and `icon`. Type, block, and port entries also contain `namespace`;
blocks have `inputs` and `outputs` arrays. Names come from the first
documentation paragraph, descriptions from `@brief`/`@details`, and icons from
`@image`. Missing documentation falls back to the declaration name and empty
description/icon strings. `namespace` is the enclosing C++ scope (`""` for global
scope); for nested types it includes the enclosing classes. Port scopes are
those of their declaring structs. IDs are unqualified declaration names; port
IDs are field names. The `types` array includes only declarations from
`src/core/types.hpp`. C++ type expressions and compiler-generated IDs are omitted.

Blocks are descendants of `Block` with default `I` and `O` template arguments.
Their ports come from those default structs; `void` produces an empty output
array. Generic implementation templates without defaults are omitted from `blocks`.
Repeated namespace declarations and implicit template instances are deduplicated.

Run the metadata checks with the same assets available:

```sh
node --test .github/scripts/generate-meta.test.mjs
```

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
