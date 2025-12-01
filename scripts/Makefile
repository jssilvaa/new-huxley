CXX := g++
CXX_FLAGS := -Wall -O2 -g -std=c++17 -Iinclude -march=native
LDFLAGS := -lsodium

SRC := scenario.cpp
EX_NAME := hey 

.PHONY: all clean 
all: $(EX_NAME)
$(EX_NAME): $(SRC)
	$(CXX) $(CXX_FLAGS) $^ -o $@ $(LDFLAGS)

clean: 
	rm -f $(EX_NAME)