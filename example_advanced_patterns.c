#include "arena.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Example: Advanced Arena Patterns

//==============================================================================
// Pattern 1: Arena Pools - Managing Collections of Arenas
//==============================================================================

typedef struct ArenaPool {
    Arena *arenas;
    int count;
    int capacity;
    size_t arena_size;
} ArenaPool;

ArenaPool pool_create(int initial_capacity, size_t arena_size) {
    ArenaPool pool = {0};
    pool.capacity = initial_capacity;
    pool.arena_size = arena_size;
    pool.arenas = malloc(sizeof(Arena) * initial_capacity);
    pool.count = 0;
    return pool;
}

Arena *pool_get_arena(ArenaPool *pool) {
    // Try to find an arena with space
    for (int i = 0; i < pool->count; i++) {
        if (arena_remaining(&pool->arenas[i]) > pool->arena_size / 4) {
            return &pool->arenas[i];
        }
    }

    // Need a new arena
    if (pool->count >= pool->capacity) {
        pool->capacity *= 2;
        pool->arenas = realloc(pool->arenas, sizeof(Arena) * pool->capacity);
    }

    pool->arenas[pool->count] = arena_create(pool->arena_size);
    return &pool->arenas[pool->count++];
}

void pool_reset_all(ArenaPool *pool) {
    for (int i = 0; i < pool->count; i++) {
        arena_reset(&pool->arenas[i]);
    }
}

void pool_destroy(ArenaPool *pool) {
    for (int i = 0; i < pool->count; i++) {
        arena_destroy(&pool->arenas[i]);
    }
    free(pool->arenas);
    pool->arenas = NULL;
    pool->count = 0;
    pool->capacity = 0;
}

void example_arena_pool(void) {
    printf("=== Example 1: Arena Pool ===\n");

    ArenaPool pool = pool_create(4, 1024); // 4 arenas, 1KB each

    // Allocate from pool - automatically gets arenas as needed
    for (int i = 0; i < 10; i++) {
        Arena *arena = pool_get_arena(&pool);
        char *data = arena_alloc(arena, 200);
        sprintf(data, "Allocation %d", i);
        printf("Allocated: %s (arena %d, remaining: %zu bytes)\n",
               data, (int)(arena - pool.arenas), arena_remaining(arena));
    }

    printf("Pool has %d arenas\n", pool.count);

    // Reset all arenas in pool
    pool_reset_all(&pool);
    printf("All arenas reset\n");

    pool_destroy(&pool);
    printf("\n");
}

//==============================================================================
// Pattern 2: String Interning with Arenas
//==============================================================================

typedef struct StringIntern {
    char **strings;
    int count;
    int capacity;
    Arena *arena;
} StringIntern;

StringIntern intern_create(Arena *arena) {
    StringIntern intern = {0};
    intern.arena = arena;
    intern.capacity = 16;
    intern.strings = arena_alloc(arena, sizeof(char*) * intern.capacity);
    intern.count = 0;
    return intern;
}

const char *intern_string(StringIntern *intern, const char *str) {
    // Check if already interned
    for (int i = 0; i < intern->count; i++) {
        if (strcmp(intern->strings[i], str) == 0) {
            return intern->strings[i];
        }
    }

    // Add new string
    if (intern->count >= intern->capacity) {
        intern->capacity *= 2;
        char **new_strings = arena_alloc(intern->arena,
                                        sizeof(char*) * intern->capacity);
        memcpy(new_strings, intern->strings, sizeof(char*) * intern->count);
        intern->strings = new_strings;
    }

    size_t len = strlen(str) + 1;
    char *interned = arena_alloc(intern->arena, len);
    memcpy(interned, str, len);
    intern->strings[intern->count++] = interned;

    return interned;
}

void example_string_interning(void) {
    printf("=== Example 2: String Interning ===\n");

    Arena arena = arena_create(4096);
    StringIntern intern = intern_create(&arena);

    const char *s1 = intern_string(&intern, "hello");
    const char *s2 = intern_string(&intern, "world");
    const char *s3 = intern_string(&intern, "hello"); // Duplicate!
    const char *s4 = intern_string(&intern, "arena");

    printf("s1: %p = '%s'\n", (void*)s1, s1);
    printf("s2: %p = '%s'\n", (void*)s2, s2);
    printf("s3: %p = '%s'\n", (void*)s3, s3);
    printf("s4: %p = '%s'\n", (void*)s4, s4);

    printf("s1 == s3: %s (same pointer!)\n", s1 == s3 ? "true" : "false");
    printf("Interned %d unique strings\n", intern.count);
    printf("Arena used: %zu bytes\n", arena.offset);

    arena_destroy(&arena);
    printf("\n");
}

//==============================================================================
// Pattern 3: Deferred Array Growth (Dynamic Arrays on Arenas)
//==============================================================================

#define ARRAY_INIT_CAP 8

typedef struct {
    int *data;
    int count;
    int capacity;
    Arena *arena;
} IntArray;

IntArray array_create(Arena *arena) {
    IntArray arr = {0};
    arr.arena = arena;
    arr.capacity = ARRAY_INIT_CAP;
    arr.data = arena_alloc(arena, sizeof(int) * arr.capacity);
    arr.count = 0;
    return arr;
}

void array_push(IntArray *arr, int value) {
    if (arr->count >= arr->capacity) {
        // Grow array
        int new_capacity = arr->capacity * 2;
        int *new_data = arena_alloc(arr->arena, sizeof(int) * new_capacity);
        memcpy(new_data, arr->data, sizeof(int) * arr->count);
        arr->data = new_data;
        arr->capacity = new_capacity;
    }

    arr->data[arr->count++] = value;
}

void example_dynamic_arrays(void) {
    printf("=== Example 3: Dynamic Arrays on Arena ===\n");

    Arena arena = arena_create(8192);

    IntArray arr = array_create(&arena);

    printf("Initial capacity: %d\n", arr.capacity);

    // Push more than initial capacity
    for (int i = 0; i < 25; i++) {
        array_push(&arr, i * i);
        if (i == 7 || i == 15 || i == 24) {
            printf("After %d pushes: capacity=%d, arena used=%zu bytes\n",
                   i + 1, arr.capacity, arena.offset);
        }
    }

    printf("Final array: [");
    for (int i = 0; i < arr.count; i++) {
        printf("%d%s", arr.data[i], i < arr.count - 1 ? ", " : "");
    }
    printf("]\n");

    arena_destroy(&arena);
    printf("\n");
}

//==============================================================================
// Pattern 4: Composition - Functions Returning Arena-Allocated Data
//==============================================================================

typedef struct {
    char *name;
    int *values;
    int value_count;
} DataSet;

DataSet load_dataset(const char *name, Arena *arena) {
    DataSet ds = {0};

    // Allocate name
    size_t name_len = strlen(name) + 1;
    ds.name = arena_alloc(arena, name_len);
    strcpy(ds.name, name);

    // Simulate loading data
    ds.value_count = 5;
    ds.values = arena_alloc(arena, sizeof(int) * ds.value_count);
    for (int i = 0; i < ds.value_count; i++) {
        ds.values[i] = (i + 1) * 10;
    }

    return ds;
}

DataSet merge_datasets(DataSet a, DataSet b, Arena *output_arena) {
    // Use scratch for temporary work
    Arena *conflicts[] = {output_arena};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Build merged name
    char *temp_name = arena_alloc(scratch.arena, 256);
    sprintf(temp_name, "%s+%s", a.name, b.name);

    DataSet merged = {0};
    size_t name_len = strlen(temp_name) + 1;
    merged.name = arena_alloc(output_arena, name_len);
    strcpy(merged.name, temp_name);

    // Merge values
    merged.value_count = a.value_count + b.value_count;
    merged.values = arena_alloc(output_arena, sizeof(int) * merged.value_count);

    memcpy(merged.values, a.values, sizeof(int) * a.value_count);
    memcpy(merged.values + a.value_count, b.values, sizeof(int) * b.value_count);

    scratch_end(scratch);
    return merged;
}

DataSet process_dataset(DataSet ds, Arena *output_arena) {
    ArenaTemp scratch = scratch_begin((Arena*[]){output_arena}, 1);

    DataSet result = {0};

    // Process name
    char *temp_name = arena_alloc(scratch.arena, 256);
    sprintf(temp_name, "processed(%s)", ds.name);

    size_t name_len = strlen(temp_name) + 1;
    result.name = arena_alloc(output_arena, name_len);
    strcpy(result.name, temp_name);

    // Process values (double them)
    result.value_count = ds.value_count;
    result.values = arena_alloc(output_arena, sizeof(int) * result.value_count);
    for (int i = 0; i < result.value_count; i++) {
        result.values[i] = ds.values[i] * 2;
    }

    scratch_end(scratch);
    return result;
}

void print_dataset(DataSet ds) {
    printf("  Dataset '%s': [", ds.name);
    for (int i = 0; i < ds.value_count; i++) {
        printf("%d%s", ds.values[i], i < ds.value_count - 1 ? ", " : "");
    }
    printf("]\n");
}

void example_composition(void) {
    printf("=== Example 4: Function Composition with Arenas ===\n");

    scratch_init(1024 * 1024);

    Arena main_arena = arena_create(8192);

    // Load datasets
    DataSet ds1 = load_dataset("sensor_a", &main_arena);
    DataSet ds2 = load_dataset("sensor_b", &main_arena);

    print_dataset(ds1);
    print_dataset(ds2);

    // Merge datasets
    DataSet merged = merge_datasets(ds1, ds2, &main_arena);
    print_dataset(merged);

    // Process merged dataset
    DataSet processed = process_dataset(merged, &main_arena);
    print_dataset(processed);

    printf("Main arena used: %zu bytes\n", main_arena.offset);

    arena_destroy(&main_arena);
    scratch_cleanup();
    printf("\n");
}

//==============================================================================
// Pattern 5: Checkpoint/Restore (Save Multiple States)
//==============================================================================

typedef struct {
    size_t offsets[8];
    int count;
} ArenaCheckpoints;

void checkpoint_push(ArenaCheckpoints *cp, Arena *arena) {
    if (cp->count < 8) {
        cp->offsets[cp->count++] = arena->offset;
    }
}

void checkpoint_pop(ArenaCheckpoints *cp, Arena *arena) {
    if (cp->count > 0) {
        arena->offset = cp->offsets[--cp->count];
    }
}

void example_checkpoints(void) {
    printf("=== Example 5: Checkpoints (Multiple Save Points) ===\n");

    Arena arena = arena_create(4096);
    ArenaCheckpoints checkpoints = {0};

    printf("Initial offset: %zu\n", arena.offset);

    // Checkpoint 1
    checkpoint_push(&checkpoints, &arena);
    int *data1 = arena_alloc(&arena, sizeof(int) * 10);
    data1[0] = 100;
    printf("After alloc 1: offset=%zu, checkpoints=%d\n",
           arena.offset, checkpoints.count);

    // Checkpoint 2
    checkpoint_push(&checkpoints, &arena);
    int *data2 = arena_alloc(&arena, sizeof(int) * 20);
    data2[0] = 200;
    printf("After alloc 2: offset=%zu, checkpoints=%d\n",
           arena.offset, checkpoints.count);

    // Checkpoint 3
    checkpoint_push(&checkpoints, &arena);
    int *data3 = arena_alloc(&arena, sizeof(int) * 30);
    data3[0] = 300;
    printf("After alloc 3: offset=%zu, checkpoints=%d\n",
           arena.offset, checkpoints.count);

    // Restore to checkpoint 2
    checkpoint_pop(&checkpoints, &arena);
    printf("After pop to checkpoint 2: offset=%zu, checkpoints=%d\n",
           arena.offset, checkpoints.count);

    // Restore to checkpoint 1
    checkpoint_pop(&checkpoints, &arena);
    printf("After pop to checkpoint 1: offset=%zu, checkpoints=%d\n",
           arena.offset, checkpoints.count);

    arena_destroy(&arena);
    printf("\n");
}

int main(void) {
    example_arena_pool();
    example_string_interning();
    example_dynamic_arrays();
    example_composition();
    example_checkpoints();

    return 0;
}
