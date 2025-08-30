# Detect OS
ifeq ($(OS),Windows_NT)
    RM = del /Q
    EXE = .exe
else
    RM = rm -f
    EXE =
endif

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

SRCS = storage_manager.c dberror.c test_assign1_1.c
OBJS = $(SRCS:.c=.o)
TARGET = test_assign1$(EXE)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	-$(RM) $(OBJS) $(TARGET) test_pagefile.bin
