# libmo

[![CI](https://github.com/esrrhs/libmo/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/libmo/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

[English](README.md) | [中文说明](README_CN.md)

`libmo` is a lightweight multi-threaded memory allocator for game servers. It exposes `malloc`/`free`-style APIs and can optionally hook the process allocator so existing `malloc` / `new` calls go through the same pool.

## Features

- Thread-local size-class freelists for small/medium allocations
- Direct `mmap` / `VirtualAlloc` path for large blocks
- Optional runtime hook for `malloc` / `free` / `realloc` / `calloc` / `memalign` (and `new` / `delete` on Linux)
- Header-only public API in `include/mo.h`

## Build

Requirements: CMake ≥ 3.12, a C++11 compiler (GCC/Clang/MSVC).

```bash
./build.sh          # Debug + tests
./build.sh release  # Release + tests
```

Or manually:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The static library is produced as `build/lib/libmo.a` (or `mo.lib` on MSVC).

## Usage

```cpp
#include "mo.h"

mo_hook();

void *p = mo_alloc(32);
size_t s = mo_memsize(p);
p = mo_realloc(p, 2 * s);
mo_free(p);

p = mo_memalign(8, 64);
mo_free(p);

p = mo_calloc(1, 64);
mo_free(p);

// After hooking, system malloc/new also use libmo:
void *q = malloc(32);
free(q);
int *arr = new int[32];
delete[] arr;

mo_restore();
```

Link against `mo` and add `include/` to your include path.

## Version

Version is defined in `include/mo.h` (`MO_VERSION`). Current version: **1.1**.

## License

MIT. See [LICENSE](LICENSE).
