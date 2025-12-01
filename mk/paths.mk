SRC_DIR := src
INC_DIR := include
BUILD_HOST := build_host
BUILD_PI := build_pi

TARGET_HOST := huxley_host
TARGET_PI := huxley_pi

PI_USER ?= root
PI_HOST ?= raspberrypi
PI_PATH ?= /etc

CLI_ENTRY := client/client.py
CLI_HOST ?= localhost
CLI_PORT ?= 8080
PYTHON ?= python3

SRC_ALL := $(wildcard $(SRC_DIR)/*.cpp)
SRC := $(SRC_ALL)