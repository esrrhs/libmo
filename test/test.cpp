#include "mo.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

static void test_basic_alloc()
{
    void *p = mo_alloc(32);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 32);
    std::memset(p, 0xAB, 32);
    mo_free(p);

    p = mo_calloc(4, 16);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 64);
    for (int i = 0; i < 64; i++) {
        assert(reinterpret_cast<unsigned char *>(p)[i] == 0);
    }
    mo_free(p);

    p = mo_memalign(64, 128);
    assert(p != nullptr);
    assert((reinterpret_cast<uintptr_t>(p) % 64) == 0);
    mo_free(p);

    p = mo_realloc(nullptr, 48);
    assert(p != nullptr);
    p = mo_realloc(p, 96);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 96);
    mo_free(p);

    assert(mo_realloc(mo_alloc(16), 0) == nullptr);
}

static void test_sizes()
{
    for (size_t i = 1; i < 4096; i += 17) {
        void *p = mo_alloc(i);
        assert(p != nullptr);
        assert(mo_memsize(p) >= i);
        std::memset(p, 0x5A, i);
        p = mo_realloc(p, i * 2);
        assert(p != nullptr);
        assert(mo_memsize(p) >= i * 2);
        mo_free(p);
    }

    // Large allocation path (beyond internal size-class cache).
    void *big = mo_alloc(256 * 1024);
    assert(big != nullptr);
    assert(mo_memsize(big) >= 256 * 1024);
    mo_free(big);
}

static void test_hook()
{
    mo_hook();

    void *p = std::malloc(32);
    assert(p != nullptr);
    std::free(p);

    int *arr = new int[32];
    assert(arr != nullptr);
    delete[] arr;

    mo_restore();
}

int main()
{
    test_basic_alloc();
    test_sizes();
    test_hook();
    std::printf("libmo tests passed\n");
    return 0;
}
