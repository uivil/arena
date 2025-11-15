CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -pthread -lm
LDFLAGS = -lm -pthread

# Object files
ARENA_OBJ = arena.o

# Targets
TARGETS = example example_temp_pattern example_scratch_arenas example_parser \
          example_per_frame example_advanced_patterns

all: $(TARGETS)

# Basic example
example: $(ARENA_OBJ) example.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Temp pattern examples
example_temp_pattern: $(ARENA_OBJ) example_temp_pattern.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Scratch arena examples
example_scratch_arenas: $(ARENA_OBJ) example_scratch_arenas.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Parser example
example_parser: $(ARENA_OBJ) example_parser.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Per-frame allocation example
example_per_frame: $(ARENA_OBJ) example_per_frame.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Advanced patterns example
example_advanced_patterns: $(ARENA_OBJ) example_advanced_patterns.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object file rules
arena.o: arena.c arena.h
	$(CC) $(CFLAGS) -c arena.c

example.o: example.c arena.h
	$(CC) $(CFLAGS) -c example.c

example_temp_pattern.o: example_temp_pattern.c arena.h
	$(CC) $(CFLAGS) -c example_temp_pattern.c

example_scratch_arenas.o: example_scratch_arenas.c arena.h
	$(CC) $(CFLAGS) -c example_scratch_arenas.c

example_parser.o: example_parser.c arena.h
	$(CC) $(CFLAGS) -c example_parser.c

example_per_frame.o: example_per_frame.c arena.h
	$(CC) $(CFLAGS) -c example_per_frame.c

example_advanced_patterns.o: example_advanced_patterns.c arena.h
	$(CC) $(CFLAGS) -c example_advanced_patterns.c

# Convenience targets
run-basic: example
	./example

run-temp: example_temp_pattern
	./example_temp_pattern

run-scratch: example_scratch_arenas
	./example_scratch_arenas

run-parser: example_parser
	./example_parser

run-per-frame: example_per_frame
	./example_per_frame

run-advanced: example_advanced_patterns
	./example_advanced_patterns

run-all: $(TARGETS)
	@echo "=== Running Basic Example ==="
	@./example
	@echo ""
	@echo "=== Running Temp Pattern Examples ==="
	@./example_temp_pattern
	@echo ""
	@echo "=== Running Scratch Arena Examples ==="
	@./example_scratch_arenas
	@echo ""
	@echo "=== Running Parser Example ==="
	@./example_parser
	@echo ""
	@echo "=== Running Per-Frame Example ==="
	@./example_per_frame
	@echo ""
	@echo "=== Running Advanced Patterns ==="
	@./example_advanced_patterns

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean run-basic run-temp run-scratch run-parser run-per-frame \
        run-advanced run-all
