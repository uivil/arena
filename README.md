# Arena Allocator in C - Complete Guide

A comprehensive implementation of arena allocator patterns in C, demonstrating modern memory management techniques for high-performance applications.

## Table of Contents

1. [What is an Arena Allocator?](#what-is-an-arena-allocator)
2. [Core API](#core-api)
3. [Advanced Patterns](#advanced-patterns)
4. [Examples](#examples)
5. [Building and Running](#building-and-running)
6. [Best Practices](#best-practices)

## What is an Arena Allocator?

An arena allocator (also known as a region-based allocator or bump allocator) is a memory management strategy where:

- A large block of memory is allocated upfront
- Individual allocations are served by bumping a pointer forward
- No individual deallocations - the entire arena is freed at once
- Very fast allocation (just pointer arithmetic)
- No fragmentation
- Perfect for temporary allocations with similar lifetimes

## Core API

### Basic Arena Operations

```c
Arena arena_create(size_t capacity);
void *arena_alloc(Arena *arena, size_t size);
void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);
void arena_reset(Arena *arena);
void arena_destroy(Arena *arena);
size_t arena_remaining(Arena *arena);
```

### Temporary Arena Pattern

The temp pattern allows you to save the arena state, make temporary allocations, then restore:

```c
ArenaTemp arena_temp_begin(Arena *arena);
void arena_temp_end(ArenaTemp temp);
```

Example:
```c
Arena arena = arena_create(1024);

int *permanent = arena_alloc(&arena, sizeof(int) * 10);

{
    ArenaTemp temp = arena_temp_begin(&arena);

    // These allocations will be freed when temp ends
    char *temporary = arena_alloc(&arena, 100);
    // ... use temporary data ...

    arena_temp_end(temp); // Frees temporary allocations
}

// permanent is still valid!
```

### Scratch Arena System

Thread-local scratch arenas for temporary work:

```c
void scratch_init(size_t arena_size);
ArenaTemp scratch_begin(Arena **conflicts, size_t conflict_count);
void scratch_end(ArenaTemp temp);
void scratch_cleanup(void);
```

The key insight: Pass your current arena as a "conflict" to get a different scratch arena:

```c
void process_data(Arena *output_arena) {
    // Get scratch that doesn't conflict with output
    Arena *conflicts[] = {output_arena};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Use scratch for temporary work
    char *temp = arena_alloc(scratch.arena, 1000);
    // ... process ...

    // Store results in output
    char *result = arena_alloc(output_arena, final_size);

    scratch_end(scratch); // Clean up temp work
}
```

## Advanced Patterns

### 1. Temporary Allocations (Temp Pattern)

Save arena state, do work, restore state. Perfect for:
- Intermediate calculations
- String building
- Temporary buffers

See: `example_temp_pattern.c`

### 2. Scratch Arenas

Thread-local arenas for temporary work. Key patterns:
- Pass arenas to functions without conflicts
- Nested function calls each get their own scratch
- Multiple scratch arenas in same function

See: `example_scratch_arenas.c`

### 3. Per-Frame Allocation (Game Loop)

Reset arena every frame for per-frame data:
```c
while (game_running) {
    arena_reset(&frame_arena);

    // Allocate all per-frame data
    ParticleSystem *particles = arena_alloc(&frame_arena, ...);
    DebugText *debug = arena_alloc(&frame_arena, ...);

    // Process frame...

    // Next iteration: reset frees everything!
}
```

See: `example_per_frame.c`

### 4. Per-Request Allocation (Server)

One arena per request, freed when request completes:
```c
void handle_request(const char *raw) {
    Arena request_arena = arena_create(64 * 1024);

    Request req = parse_request(raw, &request_arena);
    Response resp = handle_request(req, &request_arena);
    send_response(resp);

    arena_destroy(&request_arena); // Free all request data at once
}
```

See: `example_per_frame.c`

### 5. Parser with Arenas

Classic use case - different lifetimes for lexer and parser:
```c
Arena token_arena = arena_create(4096);
Arena ast_arena = arena_create(4096);

TokenList tokens = lex(input, &token_arena);
ASTNode *ast = parse(tokens, &ast_arena);

arena_destroy(&token_arena); // Tokens no longer needed
// ast_arena kept alive for later use
```

See: `example_parser.c`

### 6. Arena Pools

Manage multiple arenas automatically:
```c
ArenaPool pool = pool_create(4, 1024);

for (int i = 0; i < 100; i++) {
    Arena *arena = pool_get_arena(&pool); // Auto-grows pool
    void *data = arena_alloc(arena, size);
}

pool_reset_all(&pool);  // Reset all arenas
pool_destroy(&pool);     // Free everything
```

See: `example_advanced_patterns.c`

### 7. String Interning

Deduplicate strings using arena storage:
```c
StringIntern intern = intern_create(&arena);

const char *s1 = intern_string(&intern, "hello");
const char *s2 = intern_string(&intern, "hello");

assert(s1 == s2); // Same pointer!
```

See: `example_advanced_patterns.c`

### 8. Dynamic Arrays on Arenas

Growable arrays without manual memory management:
```c
IntArray arr = array_create(&arena);

for (int i = 0; i < 1000; i++) {
    array_push(&arr, i); // Grows automatically
}

// No need to free - arena handles it
```

See: `example_advanced_patterns.c`

### 9. Function Composition

Chain functions that allocate and return arena-allocated data:
```c
DataSet ds1 = load_dataset("data1", &arena);
DataSet ds2 = load_dataset("data2", &arena);
DataSet merged = merge_datasets(ds1, ds2, &arena);
DataSet processed = process_dataset(merged, &arena);

// All allocations in same arena, freed together
```

See: `example_advanced_patterns.c`

### 10. Checkpoints (Multiple Save Points)

Save multiple arena states:
```c
ArenaCheckpoints cp = {0};

checkpoint_push(&cp, &arena);
// ... allocations ...

checkpoint_push(&cp, &arena);
// ... more allocations ...

checkpoint_pop(&cp, &arena); // Restore to second checkpoint
checkpoint_pop(&cp, &arena); // Restore to first checkpoint
```

See: `example_advanced_patterns.c`

## Examples

This repository includes comprehensive examples:

| File | Description |
|------|-------------|
| `example.c` | Basic arena usage and concepts |
| `example_temp_pattern.c` | Temporary allocation patterns |
| `example_scratch_arenas.c` | Scratch arena system and conflict avoidance |
| `example_parser.c` | Lexer/parser with arena allocation |
| `example_per_frame.c` | Game loop and server request patterns |
| `example_advanced_patterns.c` | Arena pools, interning, dynamic arrays, etc. |

## Building and Running

### Build All Examples

```bash
make
```

### Run Individual Examples

```bash
make run-basic          # Basic arena usage
make run-temp           # Temp pattern examples
make run-scratch        # Scratch arena examples
make run-parser         # Parser example
make run-per-frame      # Per-frame/per-request patterns
make run-advanced       # Advanced patterns
```

### Run All Examples

```bash
make run-all
```

### Clean Build Artifacts

```bash
make clean
```

## Best Practices

### 1. Lifetime-Based Arena Strategy

Use different arenas for different lifetimes:
- **Permanent arena**: Lives for entire program
- **Per-frame arena**: Reset every frame
- **Per-request arena**: One per request
- **Scratch arenas**: For temporary work

### 2. Conflict Avoidance

Always pass your arena as a conflict when getting scratch:
```c
void my_function(Arena *output) {
    Arena *conflicts[] = {output};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Now scratch and output don't interfere
}
```

### 3. Temp for Intermediate Work

Use temps when you need temporary allocations within a function:
```c
char *build_string(Arena *output) {
    ArenaTemp temp = arena_temp_begin(output);

    // Temporary work buffer
    char *working = arena_alloc(output, 1000);
    // ... build string ...

    size_t final_size = calculate_size(working);
    arena_temp_end(temp);  // Free working buffer

    // Allocate exact size needed
    char *result = arena_alloc(output, final_size);
    return result;
}
```

### 4. Stack Discipline

Release temps/scratches in reverse order (LIFO):
```c
ArenaTemp temp1 = arena_temp_begin(&arena);
ArenaTemp temp2 = arena_temp_begin(&arena);

arena_temp_end(temp2);  // Release second first
arena_temp_end(temp1);  // Then first
```

### 5. Don't Mix with malloc/free

Once data is in an arena, keep it there. Don't allocate arena-managed pointers with malloc.

### 6. Size Arenas Appropriately

- Start with reasonable sizes (1MB is common)
- Profile to find optimal sizes
- Use pools if size is unpredictable

### 7. Zero-Initialize by Default

This implementation zero-initializes allocations for safety. Remove if performance-critical.

## Use Cases

Arena allocators excel in:

- **Parsing**: Lexer tokens, AST nodes with different lifetimes
- **Game engines**: Per-frame temporary data
- **Servers**: Per-request allocations
- **Compilers**: IR nodes, symbol tables per compilation unit
- **Data processing**: Batch processing with clear boundaries
- **String operations**: Building, concatenating, formatting

## Performance Characteristics

- **Allocation**: O(1) - just pointer arithmetic
- **Deallocation**: O(1) - reset pointer or free entire block
- **Memory overhead**: Minimal - no per-allocation metadata
- **Cache performance**: Excellent - allocations are contiguous
- **Fragmentation**: None - linear allocation

## Advantages Over malloc/free

- **10-100x faster allocation** than malloc
- **No fragmentation**
- **Predictable memory usage**
- **Simplified error handling** (allocation failure is arena-wide)
- **Automatic cleanup** (free entire lifetime at once)
- **Cache-friendly** (data locality)

## Limitations

- **Fixed capacity** (can't grow beyond initial size)
- **No individual frees** (all-or-nothing)
- **Wasted space** if not fully utilized
- **Not suitable** for objects with very different lifetimes

## Thread Safety

- Each arena should be used by only one thread
- Scratch arenas are thread-local (safe by default)
- For multi-threaded code, give each thread its own arenas

## Further Reading

- Ryan Fleury's "Untangling Lifetimes: The Arena Allocator"
- Ginger Bill's articles on custom allocators
- Casey Muratori's Handmade Hero memory management discussions

## License

This is free and unencumbered software released into the public domain.
