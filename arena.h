#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct Arena {
    unsigned char *buffer;
    size_t capacity;
    size_t offset;
} Arena;

// Initialize an arena with a given capacity
Arena arena_create(size_t capacity);

// Allocate memory from the arena
void *arena_alloc(Arena *arena, size_t size);

// Allocate aligned memory from the arena
void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);

// Reset the arena (doesn't free memory, just resets the offset)
void arena_reset(Arena *arena);

// Free the arena and its underlying buffer
void arena_destroy(Arena *arena);

// Get remaining capacity in the arena
size_t arena_remaining(Arena *arena);

//==============================================================================
// Temporary Arena Pattern (Scratch/Temp Allocations)
//==============================================================================

// Represents a temporary state of an arena that can be restored
typedef struct ArenaTemp {
    Arena *arena;
    size_t saved_offset;
} ArenaTemp;

// Begin a temporary allocation region (saves current state)
ArenaTemp arena_temp_begin(Arena *arena);

// End a temporary allocation region (restores saved state)
void arena_temp_end(ArenaTemp temp);

//==============================================================================
// Scratch Arena System (Thread-Local Temporary Storage)
//==============================================================================

#define MAX_SCRATCH_ARENAS 2

// Initialize the scratch arena system (call once at program startup)
void scratch_init(size_t arena_size);

// Get a scratch arena (not conflicting with any in the conflict array)
ArenaTemp scratch_begin(Arena **conflicts, size_t conflict_count);

// Release a scratch arena
void scratch_end(ArenaTemp temp);

// Cleanup scratch arenas (call at program shutdown)
void scratch_cleanup(void);

#endif // ARENA_H
