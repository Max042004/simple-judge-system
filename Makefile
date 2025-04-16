# Use g++ for compilation
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

# Directories

SRC_DIR         := main
MODEL_DIR       := $(SRC_DIR)/models
VIEW_DIR        := $(SRC_DIR)/views
CONTROLLER_DIR  := $(SRC_DIR)/controllers
PAGE_DIR		:= $(SRC_DIR)/pages

# Gather all .cpp files in the src directories
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
SOURCES += $(wildcard $(MODEL_DIR)/*.cpp)
SOURCES += $(wildcard $(VIEW_DIR)/*.cpp)
SOURCES += $(wildcard $(CONTROLLER_DIR)/*.cpp)
SOURCES += $(wildcard $(PAGE_DIR)/*.cpp)

# Convert each .cpp into a matching .o filename
OBJECTS  := $(SOURCES:.cpp=.o)

# The final executable's name
TARGET   := program

# Default rule: build everything
all: $(TARGET) $(SERVER_TARGET)

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
	rm -f $(TARGET) $(SERVER_TARGET) $(SRC_DIR)/*.o $(MODEL_DIR)/*.o $(VIEW_DIR)/*.o $(CONTROLLER_DIR)/*.o *.o

.PHONY: all clean test run-server