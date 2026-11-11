#include "mo.h"

// CMake/MSVC Release flags define NDEBUG, which would strip <cassert>
// asserts and let a failing test exit 0. Undo it in this translation unit
// so checks stay active in Debug and Release.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>

static void fill_bytes(void *p, size_t n, unsigned char seed)
{
    unsigned char *b = reinterpret_cast<unsigned char *>(p);
    for (size_t i = 0; i < n; i++) {
        b[i] = static_cast<unsigned char>(seed + i);
    }
}

static void check_bytes(const void *p, size_t n, unsigned char seed)
{
    const unsigned char *b = reinterpret_cast<const unsigned char *>(p);
    for (size_t i = 0; i < n; i++) {
        assert(b[i] == static_cast<unsigned char>(seed + i));
    }
}

// Touch first, middle, and last bytes so large blocks are checked without
// writing the entire region. Write only distinct indices: n==1 keeps a
// single first-byte pattern, and mid is used only when it is a third slot.
static void touch_ends(void *p, size_t n, unsigned char seed)
{
    if (n == 0) {
        return;
    }
    unsigned char *b = reinterpret_cast<unsigned char *>(p);
    b[0] = seed;
    if (n > 1) {
        b[n - 1] = static_cast<unsigned char>(seed + 1);
    }
    if (n > 2) {
        b[n / 2] = static_cast<unsigned char>(seed + 2);
    }
}

static void check_ends(const void *p, size_t n, unsigned char seed)
{
    if (n == 0) {
        return;
    }
    const unsigned char *b = reinterpret_cast<const unsigned char *>(p);
    assert(b[0] == seed);
    if (n > 1) {
        assert(b[n - 1] == static_cast<unsigned char>(seed + 1));
    }
    if (n > 2) {
        assert(b[n / 2] == static_cast<unsigned char>(seed + 2));
    }
}

static void init_thread_allocator()
{
    // mo_free / mo_memsize do not create the thread-local instance.
    void *warmup = mo_alloc(8);
    assert(warmup != nullptr);
    mo_free(warmup);
}

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
    fill_bytes(p, 48, 0xC2);
    p = mo_realloc(p, 96);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 96);
    check_bytes(p, 48, 0xC2);
    mo_free(p);

    assert(mo_realloc(mo_alloc(16), 0) == nullptr);
}

static void test_sizes()
{
    for (size_t i = 1; i < 4096; i += 17) {
        void *p = mo_alloc(i);
        assert(p != nullptr);
        assert(mo_memsize(p) >= i);
        fill_bytes(p, i, 0x5A);
        p = mo_realloc(p, i * 2);
        assert(p != nullptr);
        assert(mo_memsize(p) >= i * 2);
        check_bytes(p, i, 0x5A);
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

static void test_cross_thread_free()
{
    const int kSmall = 32;
    const int kLarge = 4;
    void *small[kSmall];
    size_t small_sz[kSmall];
    void *large[kLarge];
    size_t large_sz[kLarge];

    // Main thread allocates; a worker frees (small size-class and large mmap).
    for (int i = 0; i < kSmall; i++) {
        small_sz[i] = static_cast<size_t>(16 + i * 8);
        small[i] = mo_alloc(small_sz[i]);
        assert(small[i] != nullptr);
        fill_bytes(small[i], small_sz[i], static_cast<unsigned char>(0xA0 + i));
        assert(mo_memsize(small[i]) >= small_sz[i]);
    }
    for (int i = 0; i < kLarge; i++) {
        large_sz[i] = static_cast<size_t>(128 * 1024 + i * 4096);
        large[i] = mo_alloc(large_sz[i]);
        assert(large[i] != nullptr);
        assert(mo_memsize(large[i]) >= large_sz[i]);
        touch_ends(large[i], large_sz[i], static_cast<unsigned char>(0xB0 + i));
        check_ends(large[i], large_sz[i], static_cast<unsigned char>(0xB0 + i));
    }

    std::thread worker([&]() {
        init_thread_allocator();

        for (int i = 0; i < kSmall; i++) {
            check_bytes(small[i], small_sz[i], static_cast<unsigned char>(0xA0 + i));
            mo_free(small[i]);
            small[i] = nullptr;
        }
        for (int i = 0; i < kLarge; i++) {
            check_ends(large[i], large_sz[i], static_cast<unsigned char>(0xB0 + i));
            mo_free(large[i]);
            large[i] = nullptr;
        }

        // Exercise this thread after taking remote frees into its freelist.
        void *p = mo_alloc(48);
        assert(p != nullptr);
        fill_bytes(p, 48, 0xCC);
        check_bytes(p, 48, 0xCC);
        mo_free(p);
    });
    worker.join();

    // Reverse direction: worker allocates, main thread frees.
    void *from_worker[16];
    std::thread producer([&]() {
        for (int i = 0; i < 16; i++) {
            from_worker[i] = mo_alloc(48);
            assert(from_worker[i] != nullptr);
            fill_bytes(from_worker[i], 48, static_cast<unsigned char>(0x31 + i));
        }
    });
    producer.join();

    init_thread_allocator();
    for (int i = 0; i < 16; i++) {
        assert(from_worker[i] != nullptr);
        check_bytes(from_worker[i], 48, static_cast<unsigned char>(0x31 + i));
        mo_free(from_worker[i]);
    }
}

static void test_alignment()
{
    static const size_t aligns[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096};
    static const size_t sizes[] = {
        1, 7, 8, 15, 16, 31, 32, 63, 64, 100, 128, 257, 1000, 4096
    };
    const size_t nalign = sizeof(aligns) / sizeof(aligns[0]);
    const size_t nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t a = 0; a < nalign; a++) {
        const size_t align = aligns[a];
        for (size_t s = 0; s < nsizes; s++) {
            const size_t sz = sizes[s];
            void *p = mo_memalign(align, sz);
            assert(p != nullptr);
            assert((reinterpret_cast<uintptr_t>(p) % align) == 0);
            assert(mo_memsize(p) >= sz);
            fill_bytes(p, sz, 0x3C);
            check_bytes(p, sz, 0x3C);
            mo_free(p);
        }
    }

    // align == 1 is valid (every address is 1-aligned).
    void *p = mo_memalign(1, 17);
    assert(p != nullptr);
    fill_bytes(p, 17, 0x44);
    check_bytes(p, 17, 0x44);
    mo_free(p);
}

static void test_realloc_preserve()
{
    // Grow within a small size class.
    void *p = mo_alloc(24);
    assert(p != nullptr);
    fill_bytes(p, 24, 0x11);
    p = mo_realloc(p, 40);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 40);
    check_bytes(p, 24, 0x11);
    fill_bytes(static_cast<unsigned char *>(p) + 24, 16, 0x22);

    // Grow across size-class buckets.
    p = mo_realloc(p, 300);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 300);
    check_bytes(p, 24, 0x11);
    check_bytes(static_cast<unsigned char *>(p) + 24, 16, 0x22);

    p = mo_realloc(p, 5000);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 5000);
    check_bytes(p, 24, 0x11);
    check_bytes(static_cast<unsigned char *>(p) + 24, 16, 0x22);

    // Shrink: the kept prefix must be preserved. The implementation may
    // leave the reported usable size unchanged.
    p = mo_realloc(p, 64);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 64);
    check_bytes(p, 24, 0x11);
    check_bytes(static_cast<unsigned char *>(p) + 24, 16, 0x22);

    // A request still within the old usable size must also preserve.
    p = mo_realloc(p, 128);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 128);
    check_bytes(p, 24, 0x11);
    mo_free(p);

    // Grow from the size-class path onto the large mmap path, then shrink.
    const size_t small_keep = 8 * 1024;
    const size_t large_sz = 128 * 1024;
    p = mo_alloc(small_keep);
    assert(p != nullptr);
    fill_bytes(p, small_keep, 0x7A);
    p = mo_realloc(p, large_sz);
    assert(p != nullptr);
    assert(mo_memsize(p) >= large_sz);
    check_bytes(p, small_keep, 0x7A);
    fill_bytes(static_cast<unsigned char *>(p) + small_keep, 64, 0x8B);

    p = mo_realloc(p, 32 * 1024);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 32 * 1024);
    check_bytes(p, small_keep, 0x7A);
    check_bytes(static_cast<unsigned char *>(p) + small_keep, 64, 0x8B);
    mo_free(p);

    // realloc(nullptr, n) is equivalent to alloc.
    p = mo_realloc(nullptr, 64);
    assert(p != nullptr);
    assert(mo_memsize(p) >= 64);
    fill_bytes(p, 64, 0x90);
    check_bytes(p, 64, 0x90);
    mo_free(p);
}

static void test_size_boundaries()
{
    mo_free(nullptr);

    void *p = mo_alloc(0);
    if (p != nullptr) {
        mo_free(p);
    }

    p = mo_calloc(0, 32);
    if (p != nullptr) {
        mo_free(p);
    }
    p = mo_calloc(16, 0);
    if (p != nullptr) {
        mo_free(p);
    }

    p = mo_realloc(nullptr, 0);
    if (p != nullptr) {
        mo_free(p);
    }

    static const size_t edges[] = {
        1, 31, 32, 33,
        255, 256, 257,
        1023, 1024, 1025,
        4095, 4096, 4097,
        16383, 16384, 16385,
        65535, 65536, 65537,
        // Internal size-class cache stops at 100KiB (plus a small header).
        100 * 1024 - 128, 100 * 1024 - 64, 100 * 1024 - 32,
        100 * 1024 - 16, 100 * 1024 - 8, 100 * 1024 - 1,
        100 * 1024, 100 * 1024 + 1, 100 * 1024 + 8,
        100 * 1024 + 16, 100 * 1024 + 32, 100 * 1024 + 64
    };
    const size_t nedges = sizeof(edges) / sizeof(edges[0]);

    for (size_t i = 0; i < nedges; i++) {
        const size_t sz = edges[i];
        p = mo_alloc(sz);
        assert(p != nullptr);
        assert(mo_memsize(p) >= sz);
        touch_ends(p, sz, 0xE1);
        check_ends(p, sz, 0xE1);
        mo_free(p);

        p = mo_calloc(1, sz);
        assert(p != nullptr);
        assert(mo_memsize(p) >= sz);
        const unsigned char *b = reinterpret_cast<const unsigned char *>(p);
        for (size_t j = 0; j < sz; j++) {
            assert(b[j] == 0);
        }
        mo_free(p);
    }
}

int main()
{
    test_basic_alloc();
    test_sizes();
    test_hook();
    test_cross_thread_free();
    test_alignment();
    test_realloc_preserve();
    test_size_boundaries();
    std::printf("libmo tests passed\n");
    return 0;
}
