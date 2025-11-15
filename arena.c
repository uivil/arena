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

//==============================================================================
// Temporary Arena Pattern Implementation
//==============================================================================

ArenaTemp arena_temp_begin(Arena *arena) {
    ArenaTemp temp = {0};
    if (arena != NULL) {
        temp.arena = arena;
        temp.saved_offset = arena->offset;
    }
    return temp;
}

void arena_temp_end(ArenaTemp temp) {
    if (temp.arena != NULL) {
        // Restore the arena to its saved state
        temp.arena->offset = temp.saved_offset;
    }
}

//==============================================================================
// Scratch Arena System Implementation
//==============================================================================

#include <pthread.h>

// Thread-local storage for scratch arenas
static __thread Arena scratch_arenas[MAX_SCRATCH_ARENAS] = {0};
static __thread int scratch_initialized = 0;
static size_t scratch_arena_size = 0;

void scratch_init(size_t arena_size) {
    scratch_arena_size = arena_size;
}

static void ensure_scratch_initialized(void) {
    if (!scratch_initialized && scratch_arena_size > 0) {
        for (int i = 0; i < MAX_SCRATCH_ARENAS; i++) {
            scratch_arenas[i] = arena_create(scratch_arena_size);
        }
        scratch_initialized = 1;
    }
}

ArenaTemp scratch_begin(Arena **conflicts, size_t conflict_count) {
    ensure_scratch_initialized();

    // Find a scratch arena that doesn't conflict with any in the conflicts array
    for (int i = 0; i < MAX_SCRATCH_ARENAS; i++) {
        Arena *candidate = &scratch_arenas[i];

        // Check if this arena conflicts with any in the conflicts array
        int has_conflict = 0;
        for (size_t j = 0; j < conflict_count; j++) {
            if (conflicts[j] == candidate) {
                has_conflict = 1;
                break;
            }
        }

        // If no conflict, use this arena
        if (!has_conflict) {
            return arena_temp_begin(candidate);
        }
    }

    // If we get here, all arenas are in use (this shouldn't happen with proper usage)
    assert(0 && "All scratch arenas are in use - increase MAX_SCRATCH_ARENAS or fix nesting");
    ArenaTemp temp = {0};
    return temp;
}

void scratch_end(ArenaTemp temp) {
    arena_temp_end(temp);
}

void scratch_cleanup(void) {
    if (scratch_initialized) {
        for (int i = 0; i < MAX_SCRATCH_ARENAS; i++) {
            arena_destroy(&scratch_arenas[i]);
        }
        scratch_initialized = 0;
    }
}
