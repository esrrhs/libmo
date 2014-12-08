#include "mo.h"
#include "moalloc.h"

MO_API void * mo_alloc(size_t size)
{
    return falloc(size);
}

MO_API void mo_free(void * p)
{
    ffree(p);
}

MO_API size_t mo_memsize(void * p)
{
    return fmemsize(p);
}

MO_API void * mo_realloc(void * p, size_t size)
{
    return frealloc(p, size);
}

MO_API void * mo_memalign(size_t align, size_t size)
{
    return fmemalign(align, size);
}

MO_API void * mo_calloc(size_t n, size_t size)
{
    return fcalloc(n, size);
}

