/**
 * DreamShell HollySH BIOS loader
 * Memory allocation
 * (c)2025-2026 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <stddef.h>

typedef struct chunk {
    struct chunk *next;
    struct chunk *prev;
    size_t size;
    int free;
    void *data;
} *Chunk;

static void *malloc_base = NULL;
static void *malloc_pos = NULL;

/* Align size to 32 bytes */
static inline size_t align32(size_t size) {
    return (size + 31) & ~31;
}

int malloc_init(void) {
    malloc_base = (void *)ALIGN32_ADDR(loader_end);
    malloc_pos = malloc_base + align32(sizeof(struct chunk));
    Chunk b = malloc_base;
    b->next = NULL;
    b->prev = NULL;
    b->size = 0;
    b->free = 0;
    b->data = NULL;
    return 0;
}

void malloc_stat(uint32_t *free_size, uint32_t *max_free_size) {
    uint32_t exec_addr = CACHED_ADDR(APP_BIN_ADDR);

    if ((uint32_t)malloc_base < exec_addr) {
        *free_size = exec_addr - (uint32_t)malloc_pos;
    }
    else {
        *max_free_size = CACHED_ADDR(RAM_END_ADDR) - (uint32_t)malloc_base;
    }
    *max_free_size = *free_size;
}

static Chunk malloc_chunk_find(size_t s, Chunk *heap) {
    Chunk c = malloc_base;
    for (; c && (!c->free || c->size < s); *heap = c, c = c->next);
    return c;
}

static void malloc_merge_next(Chunk c) {
    c->size = c->size + c->next->size + sizeof(struct chunk);
    c->next = c->next->next;

    if (c->next) {
        c->next->prev = c;
    }
}

static void malloc_split_next(Chunk c, size_t size) {
    Chunk newc = (Chunk)((char*) c + size);
    newc->prev = c;
    newc->next = c->next;
    newc->size = c->size - size;
    newc->free = 1;
    newc->data = newc + 1;
    if (c->next) {
        c->next->prev = newc;
    }
    c->next = newc;
    c->size = size - sizeof(struct chunk);
}

void *malloc(size_t size) {
    if (!size) {
        return NULL;
    }

    size_t length = align32(size + sizeof(struct chunk));
    Chunk prev = NULL;
    Chunk c = malloc_chunk_find(size, &prev);

    if (!c) {
        Chunk newc = malloc_pos;
        malloc_pos += length;
        newc->next = NULL;
        newc->prev = prev;
        newc->size = length - sizeof(struct chunk);
        newc->data = newc + 1;
        prev->next = newc;
        c = newc;
    }
    else if (length + sizeof(size_t) < c->size) {
        malloc_split_next(c, length);
    }

    c->free = 0;
    return c->data;
}

void free(void *ptr) {
    if (!ptr || ptr < malloc_base) {
        return;
    }

    Chunk c = (Chunk) ptr - 1;

    if (c->data != ptr) {
        return;
    }

    c->free = 1;

    if (c->next && c->next->free) {
        malloc_merge_next(c);
    }
    if (c->prev->free) {
        malloc_merge_next(c = c->prev);
    }
    if (!c->next) {
        c->prev->next = NULL;
        malloc_pos -= (c->size + sizeof(struct chunk));
    }
}

void *realloc(void *ptr, size_t size) {
    void *newptr = malloc(size);

    if (newptr && ptr && ptr >= malloc_base) {
        Chunk c = (Chunk) ptr - 1;

        if (c->data == ptr) {
            size_t length = c->size > size ? size : c->size;
            memcpy(newptr, ptr, length);
            free(ptr);
        }
    }
    return newptr;
}

static size_t memory_align(size_t alignment, size_t size) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void *aligned_alloc(size_t alignment, size_t size) {

    if (!size) {
        return NULL;
    }

    Chunk prev = malloc_base;

    while (prev->next) {
        prev = prev->next;
    }

    uintptr_t potential_data_addr = (uintptr_t)malloc_pos + sizeof(struct chunk);
    uintptr_t aligned_data_addr = memory_align(alignment, potential_data_addr);
    uintptr_t newc_addr = aligned_data_addr - sizeof(struct chunk);
    uintptr_t end_of_alloc = aligned_data_addr + size;
    Chunk newc = (Chunk)newc_addr;

    prev->next = newc;
    newc->next = NULL;
    newc->prev = prev;
    newc->size = end_of_alloc - newc_addr - sizeof(struct chunk);
    newc->data = (void *)(newc + 1);
    newc->free = 0;
    malloc_pos = (void *)end_of_alloc;

    return newc->data;
}
