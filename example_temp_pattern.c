#include "arena.h"
#include <stdio.h>
#include <string.h>

// Example 1: Basic Temp Pattern - Temporary Allocations That Get Cleaned Up
void example_basic_temp(void) {
    printf("=== Example 1: Basic Temp Pattern ===\n");

    Arena arena = arena_create(1024);

    // Permanent allocation
    int *permanent = arena_alloc(&arena, sizeof(int) * 5);
    for (int i = 0; i < 5; i++) {
        permanent[i] = i * 10;
    }
    printf("Permanent data: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", permanent[i]);
    }
    printf("\n");
    printf("Offset after permanent: %zu\n", arena.offset);

    // Temporary allocation scope
    {
        ArenaTemp temp = arena_temp_begin(&arena);

        // These allocations will be cleaned up
        char *temp_buffer = arena_alloc(&arena, 100);
        sprintf(temp_buffer, "This is temporary!");
        printf("Temp buffer: %s\n", temp_buffer);

        int *temp_numbers = arena_alloc(&arena, sizeof(int) * 20);
        for (int i = 0; i < 20; i++) {
            temp_numbers[i] = i;
        }
        printf("Offset with temp data: %zu\n", arena.offset);

        // When temp goes out of scope, we restore
        arena_temp_end(temp);
    }

    printf("Offset after temp cleanup: %zu\n", arena.offset);
    printf("Permanent data still valid: %d %d %d\n\n",
           permanent[0], permanent[1], permanent[2]);

    arena_destroy(&arena);
}

// Example 2: Nested Temp Allocations
void example_nested_temps(void) {
    printf("=== Example 2: Nested Temp Allocations ===\n");

    Arena arena = arena_create(2048);

    printf("Initial offset: %zu\n", arena.offset);

    // Outer temporary scope
    ArenaTemp outer = arena_temp_begin(&arena);
    {
        char *outer_data = arena_alloc(&arena, 100);
        strcpy(outer_data, "Outer scope data");
        printf("Outer: %s, offset=%zu\n", outer_data, arena.offset);

        // Inner temporary scope
        ArenaTemp inner = arena_temp_begin(&arena);
        {
            char *inner_data = arena_alloc(&arena, 200);
            strcpy(inner_data, "Inner scope data (will be freed first)");
            printf("Inner: %s, offset=%zu\n", inner_data, arena.offset);

            arena_temp_end(inner);
        }

        printf("After inner cleanup, offset=%zu\n", arena.offset);
        printf("Outer data still accessible: %s\n", outer_data);

        arena_temp_end(outer);
    }

    printf("After outer cleanup, offset=%zu\n\n", arena.offset);

    arena_destroy(&arena);
}

// Example 3: Using Temp for String Building
char *build_greeting(Arena *arena, const char *name, int age) {
    // Use a temp for intermediate allocations
    ArenaTemp temp = arena_temp_begin(arena);

    // Allocate working buffer
    char *working = arena_alloc(arena, 256);
    sprintf(working, "Hello, %s! You are %d years old.", name, age);

    // Calculate final size needed
    size_t len = strlen(working) + 1;

    // End temp (frees the working buffer)
    arena_temp_end(temp);

    // Allocate final result (permanent)
    char *result = arena_alloc(arena, len);
    sprintf(result, "Hello, %s! You are %d years old.", name, age);

    return result;
}

void example_string_building(void) {
    printf("=== Example 3: String Building with Temps ===\n");

    Arena arena = arena_create(4096);

    char *greeting1 = build_greeting(&arena, "Alice", 30);
    char *greeting2 = build_greeting(&arena, "Bob", 25);
    char *greeting3 = build_greeting(&arena, "Charlie", 35);

    printf("%s\n", greeting1);
    printf("%s\n", greeting2);
    printf("%s\n", greeting3);

    printf("Arena used: %zu bytes\n", arena.offset);
    printf("(Without temps, would have used much more!)\n\n");

    arena_destroy(&arena);
}

// Example 4: Multiple Temp Regions in a Function
typedef struct Point {
    double x, y;
} Point;

Point *calculate_centroids(Arena *arena, Point *points, int count,
                          int *out_centroid_count) {
    // Use temp for intermediate calculations
    ArenaTemp temp = arena_temp_begin(arena);

    // Allocate temporary arrays for processing
    double *sums_x = arena_alloc(arena, sizeof(double) * count);
    double *sums_y = arena_alloc(arena, sizeof(double) * count);

    // Do some calculations (simplified)
    for (int i = 0; i < count; i++) {
        sums_x[i] = points[i].x * 2.0;
        sums_y[i] = points[i].y * 2.0;
    }

    // Calculate how many centroids we need
    int centroid_count = count / 2;
    *out_centroid_count = centroid_count;

    // End temp (free intermediate arrays)
    arena_temp_end(temp);

    // Allocate final result (permanent)
    Point *centroids = arena_alloc(arena, sizeof(Point) * centroid_count);
    for (int i = 0; i < centroid_count; i++) {
        centroids[i].x = points[i * 2].x;
        centroids[i].y = points[i * 2].y;
    }

    return centroids;
}

void example_multiple_temps(void) {
    printf("=== Example 4: Multiple Temp Regions ===\n");

    Arena arena = arena_create(8192);

    // Allocate input points
    int point_count = 10;
    Point *points = arena_alloc(&arena, sizeof(Point) * point_count);
    for (int i = 0; i < point_count; i++) {
        points[i].x = i * 1.5;
        points[i].y = i * 2.0;
    }

    printf("Created %d input points\n", point_count);

    int centroid_count;
    Point *centroids = calculate_centroids(&arena, points, point_count,
                                          &centroid_count);

    printf("Calculated %d centroids\n", centroid_count);
    for (int i = 0; i < centroid_count; i++) {
        printf("  Centroid %d: (%.2f, %.2f)\n", i, centroids[i].x, centroids[i].y);
    }

    printf("Arena used: %zu bytes\n\n", arena.offset);

    arena_destroy(&arena);
}

int main(void) {
    example_basic_temp();
    example_nested_temps();
    example_string_building();
    example_multiple_temps();

    return 0;
}
