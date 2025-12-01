# Native (host)
HOST_CXX ?= g++
HOST_CXXFLAGS := -Wall -O3 -g -std=c++17 -I$(INC_DIR)
HOST_LDFLAGS  := -lpthread -lsqlite3 -lsodium

# Raspberry Pi 
PI_CXX ?= /home/josesilvaa/buildroot/buildroot-2025.02/output/host/bin/aarch64-linux-g++
PI_CXXFLAGS := -Wall -Wextra -O3 -g -std=c++17 -I$(INC_DIR)
PI_LDFLAGS := -lpthread -lsqlite3 -lsodium