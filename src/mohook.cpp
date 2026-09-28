#include "moalloc.h"
#include "mohook.h"

uint8_t g_old_malloc_hook_mem[JMP_CODE_LEN];
uint8_t g_old_free_hook_mem[JMP_CODE_LEN];
uint8_t g_old_realloc_hook_mem[JMP_CODE_LEN];
uint8_t g_old_memalign_hook_mem[JMP_CODE_LEN];
uint8_t g_old_calloc_hook_mem[JMP_CODE_LEN];

#ifndef _WIN32
void *operator new(size_t sz)
{
    return std::malloc(sz ? sz : 1);
}

void *operator new[](size_t sz)
{
    return std::malloc(sz ? sz : 1);
}

void operator delete(void *ptr) noexcept
{
    if (ptr) {
        std::free(ptr);
    }
}

void operator delete[](void *ptr) noexcept
{
    if (ptr) {
        std::free(ptr);
    }
}

#if __cplusplus >= 201402L
void operator delete(void *ptr, size_t) noexcept
{
    if (ptr) {
        std::free(ptr);
    }
}

void operator delete[](void *ptr, size_t) noexcept
{
    if (ptr) {
        std::free(ptr);
    }
}
#endif
#endif
