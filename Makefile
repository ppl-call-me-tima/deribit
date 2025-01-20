# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Iinclude
LDFLAGS = -lssl -lcrypto -lpthread

# Target executable
TARGET = main

# Source files
SRCS = src/main.cpp

# Default target
all: $(TARGET)

# Rule to build the executable directly from source files
$(TARGET):
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) $(LDLIBS) -o $(TARGET)

# Clean up generated files
clean:
	rm -f $(TARGET)
