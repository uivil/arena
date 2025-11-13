#include "arena.h"
#include <stdio.h>
#include <string.h>

typedef struct Person {
    char *name;
    int age;
    float height;
} Person;

int main(void) {
    // Create an arena with 1MB capacity
    Arena arena = arena_create(1024 * 1024);
    if (arena.buffer == NULL) {
        fprintf(stderr, "Failed to create arena\n");
        return 1;
    }

    printf("Arena created with capacity: %zu bytes\n", arena.capacity);
    printf("Initial remaining: %zu bytes\n\n", arena_remaining(&arena));

    // Allocate some integers
    printf("Allocating 10 integers...\n");
    int *numbers = (int *)arena_alloc(&arena, 10 * sizeof(int));
    if (numbers == NULL) {
        fprintf(stderr, "Failed to allocate numbers\n");
        arena_destroy(&arena);
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        numbers[i] = i * i;
    }

    printf("Numbers: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    printf("Remaining: %zu bytes\n\n", arena_remaining(&arena));

    // Allocate a string
    printf("Allocating a string...\n");
    char *message = (char *)arena_alloc(&arena, 50);
    if (message == NULL) {
        fprintf(stderr, "Failed to allocate message\n");
        arena_destroy(&arena);
        return 1;
    }

    strcpy(message, "Hello from the arena allocator!");
    printf("Message: %s\n", message);
    printf("Remaining: %zu bytes\n\n", arena_remaining(&arena));

    // Allocate some Person structs
    printf("Allocating 3 Person structs...\n");
    Person *people = (Person *)arena_alloc(&arena, 3 * sizeof(Person));
    if (people == NULL) {
        fprintf(stderr, "Failed to allocate people\n");
        arena_destroy(&arena);
        return 1;
    }

    // Initialize people
    people[0].name = (char *)arena_alloc(&arena, 20);
    strcpy(people[0].name, "Alice");
    people[0].age = 30;
    people[0].height = 5.6f;

    people[1].name = (char *)arena_alloc(&arena, 20);
    strcpy(people[1].name, "Bob");
    people[1].age = 25;
    people[1].height = 6.0f;

    people[2].name = (char *)arena_alloc(&arena, 20);
    strcpy(people[2].name, "Charlie");
    people[2].age = 35;
    people[2].height = 5.9f;

    printf("People:\n");
    for (int i = 0; i < 3; i++) {
        printf("  %s: age=%d, height=%.1f\n",
               people[i].name, people[i].age, people[i].height);
    }
    printf("Remaining: %zu bytes\n\n", arena_remaining(&arena));

    // Test aligned allocation
    printf("Allocating with 16-byte alignment...\n");
    void *aligned_ptr = arena_alloc_aligned(&arena, 64, 16);
    if (aligned_ptr == NULL) {
        fprintf(stderr, "Failed to allocate aligned memory\n");
        arena_destroy(&arena);
        return 1;
    }
    printf("Aligned pointer: %p (should be 16-byte aligned)\n", aligned_ptr);
    printf("Alignment check: %s\n",
           ((uintptr_t)aligned_ptr % 16 == 0) ? "PASS" : "FAIL");
    printf("Remaining: %zu bytes\n\n", arena_remaining(&arena));

    // Reset the arena
    printf("Resetting arena...\n");
    arena_reset(&arena);
    printf("Remaining after reset: %zu bytes\n", arena_remaining(&arena));
    printf("(Note: All previous allocations are now invalid!)\n\n");

    // Allocate again after reset
    printf("Allocating after reset...\n");
    int *new_numbers = (int *)arena_alloc(&arena, 5 * sizeof(int));
    if (new_numbers != NULL) {
        for (int i = 0; i < 5; i++) {
            new_numbers[i] = i * 10;
        }
        printf("New numbers: ");
        for (int i = 0; i < 5; i++) {
            printf("%d ", new_numbers[i]);
        }
        printf("\n");
    }
    printf("Remaining: %zu bytes\n\n", arena_remaining(&arena));

    // Clean up
    printf("Destroying arena...\n");
    arena_destroy(&arena);
    printf("Arena destroyed successfully!\n");

    return 0;
}
