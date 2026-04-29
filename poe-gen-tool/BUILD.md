# Build Intel® Platform Ownership Endorsement Generator (Intel® POE Generator)

## Prerequisites
- CMake 3.18 or later
- C++20 compatible compiler (GCC 10+ or Clang 10+)
- A C compiler and `make` (used internally to build OpenSSL)

All dependencies (OpenSSL, cxxopts, GoogleTest) are fetched and built automatically
by CMake — no system-level development packages are required.

Supported architectures: x86_64, aarch64.

## How to build this tool

Each build method has a different working directory requirement:

| Method | Run from |
|---|---|
| `make` | Repository root (parent of `poe-gen-tool/`) |
| `cmake` | `poe-gen-tool/` (where this file resides) |

### Using make (top-level Makefile)

| Command | Description |
|---|---|
| `make` | Release build |
| `make DEBUG=1` | Debug build |
| `make test` | Build + run unit tests |
| `make deb` | Build Debian package |
| `make rpm` | Build RPM package |
| `make standalone_packages` | Build standalone release archive |
| `make clean` | Clean build outputs |
| `make distclean` | Remove entire build directory |

The `make` wrapper places build artifacts in `poe-gen-tool/build/`.

### Using CMake directly

Release build:
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

Debug build:
```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build
```

The compiled binary is placed in `build/bin/poe-gen-tool`.

#### CMake options

| Option | Required | Default | Description |
|---|---|---|---|
| `CMAKE_BUILD_TYPE` | Optional | `Release` | Build type: `Release` or `Debug` |
| `BUILD_TESTS` | Optional | `ON` | Build unit tests (`build/test/PoeGenToolTests`) |
| `POE_VERSION` | Optional | `9.9.9.9-dev` | Version string embedded in the binary |
| `FORTIFY_SOURCE_VAL` | Optional | `2` | `_FORTIFY_SOURCE` level (1, 2, or 3) |

Example with options:
```sh
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DPOE_VERSION=1.0.0.0 \
    -DBUILD_TESTS=OFF
cmake --build build
```

## Running unit tests

```sh
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
