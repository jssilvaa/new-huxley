# Ensure bash shell for advanced recipes
SHELL := /bin/bash

# --- Configuration ---
BUILD_DIR := build
CXX ?= /home/josesilvaa/buildroot/buildroot-2025.02/output/host/bin/aarch64-linux-g++
CXXFLAGS := -Wall -O2 -std=c++17 -Iinclude # -I flag looks for headers in 'include'
LDFLAGS := -lpthread -lsqlite3 -lsodium # Link against pthread, sqlite3, libsodium

# Source and Target
EXCLUDE_SRC := src/test.cpp src/Worker2.cpp
SRC_ALL := $(wildcard src/*.cpp)
SRC := $(filter-out $(EXCLUDE_SRC), $(SRC_ALL))
# Object files are named after the source files but placed in the BUILD_DIR
OBJ := $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(SRC))
TARGET := huxley

# Pi connection settings
PI_USER := root
PI_HOST := raspberrypi
PI_PATH := /etc

# Host build settings
HOST_CXX ?= g++
HOST_CXXFLAGS := -Wall -O2 -g -std=c++17 -Iinclude
HOST_LDFLAGS := -lpthread -lsqlite3 -lsodium
HOST_BUILD_DIR := host_build
HOST_TARGET := huxley_host
CLI_ENTRY := client/client.py
PYTHON ?= python3
SIM_PORT ?= 9090
SIM_DURATION ?= 20

.PHONY: all deploy run clean
.PHONY: host host-run host-clean cli cli-run cli-shell
.PHONY: test-sim

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

# --- Host build helpers ---
$(HOST_BUILD_DIR):
	mkdir -p $@

HOST_SRC := $(SRC)
HOST_OBJ := $(patsubst src/%.cpp,$(HOST_BUILD_DIR)/%.o,$(HOST_SRC))

$(HOST_BUILD_DIR)/%.o: src/%.cpp | $(HOST_BUILD_DIR)
	$(HOST_CXX) $(HOST_CXXFLAGS) -c $< -o $@

host: $(HOST_TARGET)

$(HOST_TARGET): $(HOST_OBJ)
	$(HOST_CXX) -o $@ $^ $(HOST_LDFLAGS)

host-clean:
	rm -rf $(HOST_BUILD_DIR) $(HOST_TARGET)

host-run: host
	./$(HOST_TARGET) --no-block --port 8080

# --- CLI helpers ---
cli:
	@echo "CLI script located at $(CLI_ENTRY)"

cli-run:
	python3 $(CLI_ENTRY) localhost 8080

cli-shell:
	@echo "Launch additional shells manually via: python3 $(CLI_ENTRY) <host> <port>"

# --- Local harness ---
test-sim: host
	@echo "[SIM] Starting host server on port $(SIM_PORT) for $(SIM_DURATION)s"
	@set -euo pipefail; \
	trap 'kill $$SERVER_PID >/dev/null 2>&1 || true' EXIT; \
	./$(HOST_TARGET) --port $(SIM_PORT) --duration $(SIM_DURATION) >/tmp/huxley_sim.log 2>&1 & \
	SERVER_PID=$$!; \
	sleep 1; \
	$(PYTHON) tests/cli_harness.py --host localhost --port $(SIM_PORT); \
	wait $$SERVER_PID; \
	trap - EXIT; \
	cat /tmp/huxley_sim.log