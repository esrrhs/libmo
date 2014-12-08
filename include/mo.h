#pragma once

#include <stdlib.h>

#define MO_VERSION "1.0"
#define MO_VERSION_NUM 100
#define MO_AUTHOR "esrrhs@163.com"

#define MO_API extern "C"

MO_API void * mo_alloc(size_t size);
MO_API void mo_free(void * p);
MO_API size_t mo_memsize(void * p);
MO_API void * mo_realloc(void * p, size_t size);
MO_API void * mo_memalign(size_t align, size_t size);
MO_API void * mo_calloc(size_t n, size_t size);

