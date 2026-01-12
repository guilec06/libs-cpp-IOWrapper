# Makefile for io_uring module

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
INCLUDES = -I../../include -I../../include/io_uring

# Source files
SRCS = ring_access.c ring_ressource_manager.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(OBJS)

# Compile C source files to object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS)

# Phony targets
.PHONY: all clean
