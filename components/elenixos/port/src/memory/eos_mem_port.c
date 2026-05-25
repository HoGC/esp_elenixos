#include <stddef.h>

#include "esp_heap_caps.h"

void *eos_malloc_core(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void *eos_malloc_zeroed_core(size_t size)
{
    return heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void eos_free_core(void *ptr)
{
    heap_caps_free(ptr);
}

void *eos_realloc_core(void *ptr, size_t new_size)
{
    return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void *eos_malloc_large(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void eos_free_large(void *ptr)
{
    heap_caps_free(ptr);
}
