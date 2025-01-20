# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -I/home/ppl_call_me_tima/deribit/include -lssl -lcrypto -pthread

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
