/**
 * DreamShell ISO Loader
 * Memory allocation
 * (c)2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>

enum malloc_types {
    MALLOC_TYPE_INTERNAL = 0,
    MALLOC_TYPE_KATANA = 1
};
static int malloc_type = MALLOC_TYPE_INTERNAL;
static int init_count = 0;


/**
 * @brief KATANA ingame memory allocation
 */

// FIXME: Can be different
#define KATANA_MALLOC_INDEX  0x84
#define KATANA_FREE_INDEX    0x152
#define KATANA_REALLOC_INDEX 0x20a

static uint8 *katana_malloc_root = NULL;
static const uint8 katana_malloc_key[] = {0xe6, 0x2f, 0xc6, 0x2f, 0xfc, 0x7f, 0x02, 0x00};

static int katana_malloc_init(void) {
    if (!katana_malloc_root || memcmp(katana_malloc_root, katana_malloc_key, sizeof(katana_malloc_key))) {
        katana_malloc_root = search_memory(katana_malloc_key, sizeof(katana_malloc_key));
    }
    if (katana_malloc_root) {
        malloc_type = MALLOC_TYPE_KATANA;
        LOGF("KATANA malloc initialized\n");
        return 0;
    }
    LOGF("KATANA malloc init failed\n");
    return -1;
}

static void katana_malloc_stat(uint32 *free_size, uint32 *max_free_size) {
    if (katana_malloc_root) {
        return (*(void(*)())katana_malloc_root)(free_size, max_free_size);
    } else {
        *free_size = 0;
        *max_free_size = 0;
    }
}

static void *katana_malloc(uint32 size) {
    void *mem = NULL;
    if (katana_malloc_root) {
        mem = (*(void*(*)())(katana_malloc_root + KATANA_MALLOC_INDEX))(size);
    }
    return mem;
}

static void katana_free(void *data) {
    if (data && katana_malloc_root) {
        return (*(void(*)())(katana_malloc_root + KATANA_FREE_INDEX))(data);
    }
}

static void *katana_realloc(void *data, uint32 size) {
    void *mem = NULL;
    if (data && katana_malloc_root) {
        mem = (*(void*(*)())(katana_malloc_root + KATANA_REALLOC_INDEX))(data, size);
    }
    return mem;
}


/**
 * @brief Simple memory allocation
 */

static inline size_t word_align(size_t size) {
    return size + ((sizeof(size_t) - 1) & ~(sizeof(size_t) - 1));
}

struct chunk {
    struct chunk *next, *prev;
    size_t        size;
    int           free;
    void         *data;
};

typedef struct chunk *Chunk;
static void *internal_malloc_base = NULL;
static void *internal_malloc_pos = NULL;

static int internal_malloc_init(void) {

    uint32 loader_end = loader_addr + loader_size + ISOLDR_PARAMS_SIZE + 32;
    internal_malloc_base = (void *)ALIGN32_ADDR(loader_end);
    internal_malloc_pos = NULL;

    if (IsoInfo->heap >= HEAP_MODE_SPECIFY) {

        internal_malloc_base = (void *)IsoInfo->heap;

    } else if(IsoInfo->heap == HEAP_MODE_AUTO) {

        if (loader_addr < APP_ADDR) {

            if (loader_addr >= ISOLDR_DEFAULT_ADDR_LOW
                || (IsoInfo->emu_cdda && IsoInfo->exec.type != BIN_TYPE_WINCE && IsoInfo->use_irq == 0)
                || IsoInfo->boot_mode != BOOT_MODE_DIRECT
            ) {
                internal_malloc_base = (void *)ISOLDR_DEFAULT_ADDR_HIGH - 0x8000;
            }

        } else if(loader_addr < RAM_END_ADDR) {

            if (IsoInfo->boot_mode == BOOT_MODE_DIRECT) {

                if (IsoInfo->image_type == ISOFS_IMAGE_TYPE_CSO
                    || IsoInfo->image_type == ISOFS_IMAGE_TYPE_ZSO
                    || IsoInfo->emu_cdda
                ) {
                    internal_malloc_base = (void *)(ISOLDR_DEFAULT_ADDR_LOW + 0x800);
                } else {
                    internal_malloc_base = (void *)IPBIN_ADDR;
                }
            }
        }
    }

    internal_malloc_pos = internal_malloc_base + word_align(sizeof(struct chunk));
    Chunk b = internal_malloc_base;
    b->next = NULL;
    b->prev = NULL;
    b->size = 0;
    b->free = 0;
    b->data = NULL;

    LOGF("Internal malloc initialized\n");
    malloc_type = MALLOC_TYPE_INTERNAL;
    return 0;
}

static void internal_malloc_stat(uint32 *free_size, uint32 *max_free_size) {

    uint32 exec_addr = CACHED_ADDR(IsoInfo->exec.addr);

    if ((uint32)internal_malloc_base < exec_addr) {

        *free_size = exec_addr - (uint32)internal_malloc_pos;
        *max_free_size = exec_addr - (uint32)internal_malloc_base;

    } else {

        *free_size = RAM_END_ADDR - (uint32)internal_malloc_pos;
        *max_free_size = RAM_END_ADDR - (uint32)internal_malloc_base;
    }
}

Chunk internal_malloc_chunk_find(size_t s, Chunk *heap) {
    Chunk c = internal_malloc_base;
    for (; c && (!c->free || c->size < s); *heap = c, c = c->next);
    return c;
}

void internal_malloc_merge_next(Chunk c) {
    c->size = c->size + c->next->size + sizeof(struct chunk);
    c->next = c->next->next;

    if (c->next) {
        c->next->prev = c;
    }
}

void internal_malloc_split_next(Chunk c, size_t size) {
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

void *internal_malloc(size_t size) {

    if (!size) {
        return NULL;
    }

    size_t length = word_align(size + sizeof(struct chunk));
    Chunk prev = NULL;
    Chunk c = internal_malloc_chunk_find(size, &prev);

    if (!c) {
        Chunk newc = internal_malloc_pos;
        internal_malloc_pos += length;
        newc->next = NULL;
        newc->prev = prev;
        newc->size = length - sizeof(struct chunk);
        newc->data = newc + 1;
        prev->next = newc;
        c = newc;
    } else if (length + sizeof(size_t) < c->size) {
        internal_malloc_split_next(c, length);
    }

    c->free = 0;
    return c->data;
}

void internal_free(void *ptr) {
    if (!ptr || ptr < internal_malloc_base) {
        return;
    }

    Chunk c = (Chunk) ptr - 1;

    if (c->data != ptr) {
        return;
    }

    c->free = 1;

    if (c->next && c->next->free) {
        internal_malloc_merge_next(c);
    }
    if (c->prev->free) {
        internal_malloc_merge_next(c = c->prev);
    }
    if (!c->next) {
        c->prev->next = NULL;
        internal_malloc_pos -= (c->size + sizeof(struct chunk));
    }
}

void *internal_realloc(void *ptr, size_t size) {
    void *newptr = internal_malloc(size);

    if (newptr && ptr && ptr >= internal_malloc_base) {

        Chunk c = (Chunk) ptr - 1;

        if (c->data == ptr) {
            size_t length = c->size > size ? size : c->size;
            memcpy(newptr, ptr, length);
            free(ptr);
        }
    }
    return newptr;
}

int malloc_init(void) {
    if (IsoInfo->heap == HEAP_MODE_INGAME && IsoInfo->exec.type == BIN_TYPE_KATANA) {
        if (!init_count++ || katana_malloc_init() < 0) {
            return internal_malloc_init();
        }
        return 0;
    }
    init_count++;
    return internal_malloc_init();
}

void malloc_stat(uint32 *free_size, uint32 *max_free_size) {
    if (malloc_type == MALLOC_TYPE_KATANA) {
        katana_malloc_stat(free_size, max_free_size);
    } else {
        internal_malloc_stat(free_size, max_free_size);
    }
}

uint32 malloc_heap_pos() {
    if (malloc_type == MALLOC_TYPE_KATANA) {
        return 0; // Not used
    } else {
        return (uint32)internal_malloc_pos;
    }
}

void *malloc(uint32 size) {
    if (malloc_type == MALLOC_TYPE_KATANA) {
        void *ptr = katana_malloc(size);
        if (ptr == NULL) {
            LOGFF("KATANA failed, trying internal\n");
            internal_malloc_init();
            return internal_malloc(size);
        }
        return ptr;
    } else {
        return internal_malloc(size);
    }
}

void free(void *data) {
    if (malloc_type == MALLOC_TYPE_KATANA) {
        katana_free(data);
    } else {
        internal_free(data);
    }
}

void *realloc(void *data, uint32 size) {
    if (malloc_type == MALLOC_TYPE_KATANA) {
        return katana_realloc(data, size);
    } else {
        return internal_realloc(data, size);
    }
}
