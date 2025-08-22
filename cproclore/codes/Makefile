# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -Wno-unused-parameter -Wno-implicit-function-declaration -Wno-unused-variable

# Source files
SRCS = main.c hop.c reveal.c proclore.c seek.c echo.c log.c activities.c neonate.c alias.c iman.c 

# Object files
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = shell

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Debug build with extra debug information
debug: CFLAGS += -DDEBUG -g
debug: clean all
