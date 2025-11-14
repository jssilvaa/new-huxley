#############################################
# Test Makefile for huxleyserver components #
#############################################

CXX := /home/josesilvaa/buildroot/buildroot-2025.02/output/host/bin/aarch64-linux-g++
HOST_CXX ?= g++
BUILD_DIR := build
CXXFLAGS := -Wall -O2 -std=c++17 -I../include
LDFLAGS := -lpthread -lsqlite3 -lsodium

PI_USER := root
PI_HOST := raspberrypi
PI_PATH := /etc/huxley_tests

TARGET_AUTH := $(BUILD_DIR)/test_auth
TARGET_DB   := $(BUILD_DIR)/test_database
TARGET_SIM  := $(BUILD_DIR)/test_sim

.PHONY: all clean deploy run test_auth test_database sim host-sim

all: $(TARGET_AUTH) $(TARGET_DB)

$(BUILD_DIR):
	mkdir -p $@

# Direct build commands keep it simple (no object reuse optimization yet)
$(TARGET_AUTH): test_auth.cpp ../src/DatabaseEngine.cpp ../src/AuthManager.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TARGET_DB): test_database.cpp ../src/DatabaseEngine.cpp ../src/AuthManager.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

SIM_SRC := \
	../src/HuxleyServer.cpp \
	../src/WorkerThread.cpp \
	../src/SingleWorker.cpp \
	../src/MessageRouter.cpp \
	../src/AuthManager.cpp \
	../src/DatabaseEngine.cpp \
	../src/CryptoEngine.cpp \
	../src/StatusManager.cpp \
	../src/ProtocolHandler.cpp \
	../src/ClientState.cpp

$(TARGET_SIM): sim_server.cpp $(SIM_SRC) | $(BUILD_DIR)
	$(HOST_CXX) -Wall -O0 -g -std=c++17 -I../include -o $@ $^ -lpthread -lsqlite3 -lsodium

test_auth: $(TARGET_AUTH)
test_database: $(TARGET_DB)

deploy: all
	ssh $(PI_USER)@$(PI_HOST) "mkdir -p $(PI_PATH)" || exit 1
	scp $(TARGET_AUTH) $(TARGET_DB) $(PI_USER)@$(PI_HOST):$(PI_PATH)

run: deploy
	ssh $(PI_USER)@$(PI_HOST) "$(PI_PATH)/test_auth && $(PI_PATH)/test_database"

sim: $(TARGET_SIM)

host-sim: sim
	./$(TARGET_SIM)

clean:
	rm -rf $(BUILD_DIR)

#############################################
# Usage:
#   make -C tests            (build tests)
#   make -C tests run        (build, deploy, run remotely)
# Override host: make -C tests PI_HOST=192.168.1.50 run
#############################################