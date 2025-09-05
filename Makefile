ifeq ($(OS),Windows_NT)
    DELETE = del /Q
    EXT    = .exe
else
    DELETE = rm -f
    EXT    =
endif

CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -g

SOURCES  = storage_manager.c dberror.c test_assign1_1.c
OBJECTS  = $(SOURCES:.c=.o)
PROGRAM  = test_assign1$(EXT)

all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PROGRAM)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(PROGRAM)
	./$(PROGRAM)

clean:
	-$(DELETE) $(OBJECTS) $(PROGRAM) test_pagefile.bin
