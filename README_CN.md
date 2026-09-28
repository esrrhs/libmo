# libmo

[![CI](https://github.com/esrrhs/libmo/actions/workflows/ci.yml/badge.svg)](https://github.com/esrrhs/libmo/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

[English](README.md) | [中文说明](README_CN.md)

`libmo` 是面向游戏服务器的轻量多线程内存分配库，提供类似 `malloc`/`free` 的接口，并可选 hook 进程分配器，让现有的 `malloc` / `new` 也走同一套内存池。

## 特性

- 小/中等对象使用线程本地 size-class freelist
- 大块直接走 `mmap` / `VirtualAlloc`
- 可选 hook：`malloc` / `free` / `realloc` / `calloc` / `memalign`（Linux 下同时覆盖 `new` / `delete`）
- 对外头文件：`include/mo.h`

## 编译

依赖：CMake ≥ 3.12，C++11 编译器（GCC/Clang/MSVC）。

```bash
./build.sh          # Debug + 测试
./build.sh release  # Release + 测试
```

或手动：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

静态库输出为 `build/lib/libmo.a`（MSVC 下为 `mo.lib`）。

## 使用

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

// hook 之后，系统 malloc/new 也会走 libmo：
void *q = malloc(32);
free(q);
int *arr = new int[32];
delete[] arr;

mo_restore();
```

链接 `mo`，并把 `include/` 加入头文件搜索路径。

## 版本

版本写在 `include/mo.h`（`MO_VERSION`）。当前版本：**1.1**。

## 许可证

MIT，见 [LICENSE](LICENSE)。
