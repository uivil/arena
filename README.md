# Arena Allocator in C

A simple and efficient arena allocator implementation in C.

## What is an Arena Allocator?

An arena allocator (also known as a region-based allocator or bump allocator) is a memory management strategy where:
- A large block of memory is allocated upfront
- Individual allocations are served by bumping a pointer forward
- No individual deallocations - the entire arena is freed at once
- Very fast allocation (just pointer arithmetic)
- No fragmentation
- Perfect for temporary allocations with similar lifetimes

## Features

- Simple API with only 6 functions
- Aligned memory allocation support
- Zero-initialization of allocated memory
- Arena reset capability (for reusing the same buffer)
- Thread-safe when each thread has its own arena

## Building

```bash
make
```

## Running the Example

```bash
make run
```

Or directly:

```bash
./example
```

## Usage

```c
#include "arena.h"

// Create an arena with 1MB capacity
Arena arena = arena_create(1024 * 1024);

// Allocate memory
int *numbers = arena_alloc(&arena, 10 * sizeof(int));
char *string = arena_alloc(&arena, 100);

// Allocate with specific alignment
void *aligned = arena_alloc_aligned(&arena, 64, 16);

// Check remaining space
size_t remaining = arena_remaining(&arena);

// Reset the arena (invalidates all previous allocations)
arena_reset(&arena);

// Free the entire arena
arena_destroy(&arena);
```

## API Reference

### `Arena arena_create(size_t capacity)`
Creates a new arena with the specified capacity in bytes.

### `void *arena_alloc(Arena *arena, size_t size)`
Allocates `size` bytes from the arena with default alignment (pointer size).
Returns NULL if there's not enough space.

### `void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment)`
Allocates `size` bytes from the arena with the specified alignment.
Alignment must be a power of 2. Returns NULL if there's not enough space.

### `void arena_reset(Arena *arena)`
Resets the arena offset to 0, effectively "freeing" all allocations.
Note: This doesn't actually free memory, just allows reuse of the buffer.

### `void arena_destroy(Arena *arena)`
Frees the arena's underlying buffer and resets all fields.

### `size_t arena_remaining(Arena *arena)`
Returns the number of bytes remaining in the arena.

## Use Cases

Arena allocators are ideal for:
- Parsing (lexer/parser temporary data)
- Per-frame allocations in games
- Request handling in servers
- Temporary data structures
- Any scenario where many allocations have similar lifetimes

## Advantages

- **Fast**: O(1) allocation, just pointer arithmetic
- **Simple**: No complex bookkeeping
- **Cache-friendly**: Allocations are contiguous
- **No fragmentation**: Memory is used linearly
- **Predictable**: Known memory usage upfront

## Limitations

- Fixed capacity (cannot grow)
- Cannot free individual allocations
- Wastes memory if not fully utilized
- Not suitable for long-lived objects with different lifetimes
