#include "arena.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Example 1: Basic Scratch Arena Usage
void example_basic_scratch(void) {
    printf("=== Example 1: Basic Scratch Arena ===\n");

    // Get a scratch arena (no conflicts)
    ArenaTemp scratch = scratch_begin(NULL, 0);

    // Use it for temporary work
    char *buffer = arena_alloc(scratch.arena, 256);
    sprintf(buffer, "Temporary computation result: %d", 42 * 137);
    printf("%s\n", buffer);

    int *temp_array = arena_alloc(scratch.arena, sizeof(int) * 100);
    for (int i = 0; i < 100; i++) {
        temp_array[i] = i * i;
    }
    printf("Sum of squares: %d\n", temp_array[50]);

    // Release the scratch arena
    scratch_end(scratch);

    printf("Scratch arena released (memory reusable)\n\n");
}

// Example 2: Avoiding Conflicts - The Key Pattern!
// When you pass an arena to a function, that function can get its own
// scratch arena that doesn't conflict with yours
void helper_function(Arena *input_arena) {
    // Get a scratch that doesn't conflict with input_arena
    Arena *conflicts[] = {input_arena};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Now we can safely use both arenas without interference
    char *temp = arena_alloc(scratch.arena, 100);
    sprintf(temp, "Helper using scratch");
    printf("  %s\n", temp);

    char *input = arena_alloc(input_arena, 50);
    sprintf(input, "Helper using input");
    printf("  %s\n", input);

    scratch_end(scratch);
}

void example_conflict_avoidance(void) {
    printf("=== Example 2: Conflict Avoidance ===\n");

    // Get a scratch arena
    ArenaTemp scratch = scratch_begin(NULL, 0);

    char *data = arena_alloc(scratch.arena, 100);
    sprintf(data, "Main function data");
    printf("%s\n", data);

    // Pass our arena to helper - it will get a different scratch arena
    helper_function(scratch.arena);

    // Our data is still valid
    printf("%s (still valid!)\n", data);

    scratch_end(scratch);
    printf("\n");
}

// Example 3: String Processing with Scratch Arenas
char *process_string(const char *input, Arena *output_arena) {
    // Get scratch for intermediate work, avoiding output_arena
    Arena *conflicts[] = {output_arena};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Use scratch for temporary processing
    size_t len = strlen(input);
    char *upper = arena_alloc(scratch.arena, len + 1);
    for (size_t i = 0; i < len; i++) {
        upper[i] = (input[i] >= 'a' && input[i] <= 'z')
                   ? input[i] - 32 : input[i];
    }
    upper[len] = '\0';

    char *reversed = arena_alloc(scratch.arena, len + 1);
    for (size_t i = 0; i < len; i++) {
        reversed[i] = upper[len - 1 - i];
    }
    reversed[len] = '\0';

    // Allocate final result in output arena
    char *result = arena_alloc(output_arena, len + 20);
    sprintf(result, "Result: %s", reversed);

    // Clean up scratch
    scratch_end(scratch);

    return result;
}

void example_string_processing(void) {
    printf("=== Example 3: String Processing ===\n");

    Arena output = arena_create(4096);

    char *result1 = process_string("hello world", &output);
    char *result2 = process_string("arena allocator", &output);
    char *result3 = process_string("scratch patterns", &output);

    printf("%s\n", result1);
    printf("%s\n", result2);
    printf("%s\n", result3);

    printf("Output arena used: %zu bytes\n\n", output.offset);

    arena_destroy(&output);
}

// Example 4: Nested Function Calls with Scratch Arenas
typedef struct {
    char *first_name;
    char *last_name;
    char *full_name;
} Person;

char *format_name(const char *first, const char *last, Arena *output) {
    Arena *conflicts[] = {output};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    // Use scratch for temporary formatting
    size_t total_len = strlen(first) + strlen(last) + 2;
    char *temp = arena_alloc(scratch.arena, total_len);
    sprintf(temp, "%s %s", first, last);

    // Copy to output
    char *result = arena_alloc(output, total_len);
    strcpy(result, temp);

    scratch_end(scratch);
    return result;
}

Person create_person(const char *first, const char *last, Arena *output) {
    // We're using output arena, so avoid it in scratch
    Arena *conflicts[] = {output};
    ArenaTemp scratch = scratch_begin(conflicts, 1);

    Person person = {0};

    // Allocate name components in output
    person.first_name = arena_alloc(output, strlen(first) + 1);
    strcpy(person.first_name, first);

    person.last_name = arena_alloc(output, strlen(last) + 1);
    strcpy(person.last_name, last);

    // format_name will get its own scratch (different from ours)
    person.full_name = format_name(first, last, output);

    scratch_end(scratch);
    return person;
}

void example_nested_calls(void) {
    printf("=== Example 4: Nested Function Calls ===\n");

    Arena people_arena = arena_create(8192);

    Person p1 = create_person("Alice", "Smith", &people_arena);
    Person p2 = create_person("Bob", "Jones", &people_arena);
    Person p3 = create_person("Charlie", "Brown", &people_arena);

    printf("Person 1: %s (first=%s, last=%s)\n",
           p1.full_name, p1.first_name, p1.last_name);
    printf("Person 2: %s (first=%s, last=%s)\n",
           p2.full_name, p2.first_name, p2.last_name);
    printf("Person 3: %s (first=%s, last=%s)\n",
           p3.full_name, p3.first_name, p3.last_name);

    printf("People arena used: %zu bytes\n\n", people_arena.offset);

    arena_destroy(&people_arena);
}

// Example 5: Multiple Scratch Arenas in Same Function
void example_multiple_scratches(void) {
    printf("=== Example 5: Multiple Scratch Arenas ===\n");

    // We can use multiple scratch arenas at once!
    // The conflict system ensures they don't interfere

    ArenaTemp scratch1 = scratch_begin(NULL, 0);
    printf("Got scratch1\n");

    // Get scratch2, avoiding scratch1
    Arena *conflicts1[] = {scratch1.arena};
    ArenaTemp scratch2 = scratch_begin(conflicts1, 1);
    printf("Got scratch2 (different from scratch1)\n");

    // Use both
    char *data1 = arena_alloc(scratch1.arena, 100);
    sprintf(data1, "Data in scratch 1");

    char *data2 = arena_alloc(scratch2.arena, 100);
    sprintf(data2, "Data in scratch 2");

    printf("%s\n", data1);
    printf("%s\n", data2);

    // Release in reverse order (stack discipline)
    scratch_end(scratch2);
    printf("Released scratch2\n");

    scratch_end(scratch1);
    printf("Released scratch1\n\n");
}

int main(void) {
    // Initialize scratch arena system
    scratch_init(1024 * 1024); // 1MB per scratch arena

    example_basic_scratch();
    example_conflict_avoidance();
    example_string_processing();
    example_nested_calls();
    example_multiple_scratches();

    // Cleanup
    scratch_cleanup();

    return 0;
}
