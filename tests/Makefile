#############################################
# Test Makefile for huxleyserver components #
#############################################

CXX := /home/josesilvaa/buildroot/buildroot-2025.02/output/host/bin/aarch64-linux-g++
BUILD_DIR := build
CXXFLAGS := -Wall -O2 -std=c++17 -I../include
LDFLAGS := -lpthread -lsqlite3

PI_USER := root
PI_HOST := raspberrypi
PI_PATH := /etc/huxley_tests

TARGET_AUTH := $(BUILD_DIR)/test_auth
TARGET_DB   := $(BUILD_DIR)/test_database

.PHONY: all clean deploy run test_auth test_database

all: $(TARGET_AUTH) $(TARGET_DB)

$(BUILD_DIR):
	mkdir -p $@

# Direct build commands keep it simple (no object reuse optimization yet)
$(TARGET_AUTH): test_auth.cpp ../src/Database.cpp ../src/AuthManager.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TARGET_DB): test_database.cpp ../src/Database.cpp ../src/AuthManager.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test_auth: $(TARGET_AUTH)
test_database: $(TARGET_DB)

deploy: all
	ssh $(PI_USER)@$(PI_HOST) "mkdir -p $(PI_PATH)" || exit 1
	scp $(TARGET_AUTH) $(TARGET_DB) $(PI_USER)@$(PI_HOST):$(PI_PATH)

run: deploy
	ssh $(PI_USER)@$(PI_HOST) "$(PI_PATH)/test_auth && $(PI_PATH)/test_database"

clean:
	rm -rf $(BUILD_DIR)

#############################################
# Usage:
#   make -C tests            (build tests)
#   make -C tests run        (build, deploy, run remotely)
# Override host: make -C tests PI_HOST=192.168.1.50 run
#############################################