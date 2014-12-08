#include "moalloc.h"
#include "mohook.h"

uint8_t g_old_malloc_hook_mem[JMP_CODE_LEN];
uint8_t g_old_free_hook_mem[JMP_CODE_LEN];
uint8_t g_old_realloc_hook_mem[JMP_CODE_LEN];
uint8_t g_old_memalign_hook_mem[JMP_CODE_LEN];
uint8_t g_old_calloc_hook_mem[JMP_CODE_LEN];

#ifndef WIN32 
void *operator new (size_t sz)
{
    return (void *) malloc (sz ? sz : 1);
}

void *operator new[] (size_t sz)
{
    return (void *) malloc (sz ? sz : 1);
}

void operator delete (void *ptr)
{
    if (ptr)
        free(ptr);
}

void operator delete[] (void *ptr) throw ()
{
    if (ptr)
        free(ptr);
}
#endif

