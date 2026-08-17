CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

SRC_DIR  := src
OBJ_DIR  := build
TARGET   := mathio_test

# Automatically gather all .cpp source files in src/
SRCS     := $(wildcard $(SRC_DIR)/*.cpp)
OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create build folder if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all run clean