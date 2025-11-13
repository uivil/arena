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

#endif // ARENA_H
