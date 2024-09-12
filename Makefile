# Compiler to use
CC = gcc

# Flags for the compiler
CFLAGS = -I.

# Name of the executable to create
TARGET = test_assign1

# Source files
SOURCES = test_assign1_1.c storage_mgr.c dberror.c

# Object files (replace .c from SOURCES with .o)
OBJECTS = $(SOURCES:.c=.o)

# The first rule is the one that is executed when no parameters are fed into the makefile.
# This rule depends on the TARGET and will run the rules it depends on before executing.
all: $(TARGET)

# Rule for linking the final executable.
# Depends on the OBJECTS, all of which will be made automatically from the .c files.
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS)

# Rule for making the object files.
# The -c flag is for generating the object files, -o for specifying the output file.
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Rule for cleaning up the project - removing object files and the executable.
clean:
	rm -f $(OBJECTS) $(TARGET)