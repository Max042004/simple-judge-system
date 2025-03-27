# Use g++ for compilation
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

# Name of the final executable
TARGET = main

# Default rule: build everything
all: $(TARGET)

# Build rule: from main.cpp -> executable
$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

# 'make test' runs the compiled program through a Python trace script
test: all
	python3 trace_script.py

# New "check-massif" rule
check-massif: main
	valgrind --tool=massif --massif-out-file=massif.out ./main < traces/trace-1.cmd
	ms_print massif.out

# Clean up
clean:
	rm -f $(TARGET)

.PHONY: all clean test
