# Use g++ for compilation
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

# Gather all .cpp files in ./main/
SOURCES  := $(wildcard main/*.cpp)
# Convert each .cpp into a matching .o filename
OBJECTS  := $(SOURCES:.cpp=.o)

# The final executable's name
TARGET   := program

# Default rule: build everything
all: $(TARGET)

# Link all object files into the final executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile each .cpp into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# 'make test' runs the compiled program through a Python trace script
test: $(TARGET)
	python3 trace_script.py

# New "check-massif" rule
check-massif: main
	@echo "=== Massif Test Run ===" >> test_output.txt
	valgrind --tool=massif --massif-out-file=massif.out ./main < traces/trace-2.cmd >> test_output.txt 2>&1
	@echo "=== ms_print massif.out ===" >> test_output.txt
	ms_print massif.out


# Clean up
clean:
	rm -f $(TARGET) main/*.o

.PHONY: all clean test
