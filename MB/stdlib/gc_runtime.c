#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct GcBlock {
    struct GcBlock *next;
    size_t size;
    int marked;
    int scanned;
    max_align_t alignment;
    unsigned char data[];
} GcBlock;

static GcBlock *blocks;
static void ***root_slots;
static size_t root_count, root_capacity, live_bytes;
static size_t next_collection = 64u * 1024u;

static GcBlock *find_block(const void *candidate) {
    uintptr_t address = (uintptr_t)candidate;
    for (GcBlock *b = blocks; b; b = b->next) {
        uintptr_t begin = (uintptr_t)b->data;
        uintptr_t end = begin + b->size;
        if (address >= begin && address < end) return b;
    }
    return NULL;
}

static void mark_pointer(const void *value) {
    GcBlock *b = find_block(value);
    if (b) b->marked = 1;
}

void gc_collect(void) {
    for (GcBlock *b = blocks; b; b = b->next) b->marked = b->scanned = 0;
    for (size_t i = 0; i < root_count; i++) {
        void *value = *root_slots[i];
        mark_pointer(value);
    }
    int progress;
    do {
        progress = 0;
        for (GcBlock *b = blocks; b; b = b->next) {
            if (!b->marked || b->scanned) continue;
            b->scanned = 1;
            progress = 1;
            for (size_t offset = 0; offset + sizeof(void *) <= b->size; offset += sizeof(void *)) {
                void *candidate = NULL;
                memcpy(&candidate, b->data + offset, sizeof(candidate));
                mark_pointer(candidate);
            }
        }
    } while (progress);

    GcBlock **link = &blocks;
    live_bytes = 0;
    while (*link) {
        GcBlock *b = *link;
        if (!b->marked) {
            *link = b->next;
            free(b);
        } else {
            live_bytes += b->size;
            link = &b->next;
        }
    }
    next_collection = live_bytes > (SIZE_MAX - 65536u) / 2 ? SIZE_MAX : live_bytes * 2 + 65536u;
}

void *gc_alloc(int64_t requested_size) {
    if (requested_size <= 0) return NULL;
    size_t size = (size_t)requested_size;
    if (size > SIZE_MAX - sizeof(GcBlock)) return NULL;
    if (live_bytes + size > next_collection) gc_collect();
    GcBlock *b = (GcBlock *)calloc(1, sizeof(GcBlock) + size);
    if (!b) return NULL;
    b->size = size;
    b->next = blocks;
    blocks = b;
    live_bytes += size;
    return b->data;
}

void gc_root(void *pointer_slot) {
    if (!pointer_slot) return;
    void **slot = (void **)pointer_slot;
    for (size_t i = 0; i < root_count; i++) if (root_slots[i] == slot) return;
    if (root_count == root_capacity) {
        size_t capacity = root_capacity ? root_capacity * 2 : 16;
        void ***grown = (void ***)realloc(root_slots, capacity * sizeof(*grown));
        if (!grown) return;
        root_slots = grown;
        root_capacity = capacity;
    }
    root_slots[root_count++] = slot;
}

void gc_unroot(void *pointer_slot) {
    void **slot = (void **)pointer_slot;
    for (size_t i = 0; i < root_count; i++) {
        if (root_slots[i] == slot) {
            root_slots[i] = root_slots[--root_count];
            return;
        }
    }
}
