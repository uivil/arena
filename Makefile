CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = example
OBJS = arena.o example.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

arena.o: arena.c arena.h
	$(CC) $(CFLAGS) -c arena.c

example.o: example.c arena.h
	$(CC) $(CFLAGS) -c example.c

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
