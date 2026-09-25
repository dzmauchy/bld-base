# BASE block library (C++)

A header-only C++ library. Exposed types, namespaces, blocks, ports, and config carry an XML doc comment that core reads from the clang AST. Production sources are headers; the CMake target builds those headers and the native tests.

The project version is `VERSION` in `CMakeLists.txt`.

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
- `base/` — this library's push blocks, including the f32 and f64 endpoints
- `base.hpp` — includes those endpoints (`#include <base.hpp>`)

The versioned release archive contains `base.hpp`, `base/`, `core/`, `core/math/`, `browser/`, and `mcu/`.

Platform hosts are headers too:

- `src/browser/host.hpp` — browser simulation host (`#include <browser/host.hpp>`)
- `src/mcu/hal.hpp` — bare-metal timer and GPIO host (`#include <mcu/hal.hpp>`)

Pushes to `main` publish those headers as a GitHub Release tagged with the project version (`v0.1.0`). Each commit on `main` replaces that release so the tag always matches the current sources for that version. A new version in `CMakeLists.txt` publishes a new tag and leaves the previous release in place.
