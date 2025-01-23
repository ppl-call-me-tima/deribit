# Compiler and Flags
CXX = g++
CXXFLAGS = -Iinclude
LDFLAGS = -lssl -lcrypto -lpthread

TARGET = main

SRCS = src/main.cpp

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) $(LDLIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
