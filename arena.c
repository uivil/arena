#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Helper function to align a value to the next multiple of alignment
static size_t align_forward(size_t ptr, size_t alignment) {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0); // Power of 2 check
    size_t modulo = ptr & (alignment - 1);
    if (modulo != 0) {
        ptr += alignment - modulo;
    }
    return ptr;
}

Arena arena_create(size_t capacity) {
    Arena arena = {0};
    arena.buffer = (unsigned char *)malloc(capacity);
    if (arena.buffer != NULL) {
        arena.capacity = capacity;
        arena.offset = 0;
    }
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    return arena_alloc_aligned(arena, size, sizeof(void *));
}

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment) {
    if (arena == NULL || arena->buffer == NULL) {
        return NULL;
    }

    // Align the current offset
    size_t aligned_offset = align_forward(arena->offset, alignment);

    // Check if we have enough space
    if (aligned_offset + size > arena->capacity) {
        return NULL; // Out of memory
    }

    // Get pointer to the allocated memory
    void *ptr = arena->buffer + aligned_offset;

    // Update the offset
    arena->offset = aligned_offset + size;

    // Zero-initialize the allocated memory
    memset(ptr, 0, size);

    return ptr;
}

void arena_reset(Arena *arena) {
    if (arena != NULL) {
        arena->offset = 0;
    }
}

void arena_destroy(Arena *arena) {
    if (arena != NULL && arena->buffer != NULL) {
        free(arena->buffer);
        arena->buffer = NULL;
        arena->capacity = 0;
        arena->offset = 0;
    }
}

size_t arena_remaining(Arena *arena) {
    if (arena == NULL || arena->buffer == NULL) {
        return 0;
    }
    return arena->capacity - arena->offset;
}
