# Compiler
CXX := g++

# Flags
CXXFLAGS := -Wall -Wextra -std=c++17 -fopenmp -g
LDFLAGS := -static

# Directories
SRC_DIR := src
BUILD_DIR := build
TARGET := executable

# Source files
SRCS := $(wildcard $(SRC_DIR)/**/*.cpp)

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Dependency files
DEPS := $(OBJS:.o=.d)

# Include directories
INC_DIRS := $(sort $(dir $(wildcard $(SRC_DIR)/**/)))
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# Main target
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Include dependencies
-include $(DEPS)

.PHONY: clean

