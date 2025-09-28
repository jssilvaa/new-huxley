# --- Configuration ---
BUILD_DIR := build
CXX := /home/josesilvaa/buildroot/buildroot-2025.02/output/host/bin/aarch64-linux-g++
CXXFLAGS := -Wall -O2 -std=c++17 -Iinclude # -I flag looks for headers in 'include'
# LDFLAGS := -lpthread -lsqlite3 # Link against pthread and sqlite3

# Source and Target
SRC := $(wildcard src/*.cpp) # All .cpp files in src/
# Object files are named after the source files but placed in the BUILD_DIR
OBJ := $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(SRC))
TARGET := huxley

# Pi connection settings
PI_USER := root
PI_HOST := raspberrypi
PI_PATH := /etc

.PHONY: all deploy run clean

# --- Main Targets ---

all: $(TARGET)

# 1. Link the executable (linking objects into the TARGET)
$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# 2. Rule to ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $@

# 3. Rule to compile source into object files (Compilation)
# Pattern rule to turn src/FILE.cpp into build/FILE.o
$(BUILD_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Deployment and Clean ---

deploy: $(TARGET)
	scp $(TARGET) $(PI_USER)@$(PI_HOST):$(PI_PATH)

run: deploy
	ssh $(PI_USER)@$(PI_HOST) "$(PI_PATH)/$(TARGET)"

clean:
	rm -rf $(BUILD_DIR) $(TARGET)