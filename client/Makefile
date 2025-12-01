CXX ?= g++
CXXFLAGS := -Wall -Wextra -O2 -g -std=c++17 -I../include
LDFLAGS := -lpthread -lsqlite3 -lsodium

CLI_SOURCES := client.cpp main.cpp
CLI_TARGET := huxley_cli

.PHONY: all clean run

all: $(CLI_TARGET)

$(CLI_TARGET): $(CLI_SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

run: $(CLI_TARGET)
	./$(CLI_TARGET)

clean:
	rm -f $(CLI_TARGET)
