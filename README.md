# BASE block library (C++)

A header-only C++ library. Exposed types, namespaces, blocks, ports, and config carry a JSON comment that core reads from the clang AST. Production sources are headers; the CMake target builds those headers and the native tests.

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

`cmake --install build --prefix dist` installs the public headers under `dist/include`:

- `bld.hpp`, `base.hpp`, and `bld/callback.hpp` — core umbrella and member adapters
- `core/` — HAL and runtime interfaces (`Block`, `Callback`, `Array`, `Maybe`, `move()`)
- `blocks/` — platform-agnostic push blocks, including the f32 and f64 endpoints
- `math/trig.hpp` — `wrapTwoPi` and trigonometry helpers

Platform code is not part of the header set:

- `arch/wasm/host.cpp` — WASM simulation host
- `arch/mcu/hal.cpp` — bare-metal timer and GPIO host

Pushes to `main` publish those headers as a GitHub Release tagged with the project version (`v0.1.0`). Each commit on `main` replaces that release so the tag always matches the current sources for that version. A new version in `CMakeLists.txt` publishes a new tag and leaves the previous release in place.
