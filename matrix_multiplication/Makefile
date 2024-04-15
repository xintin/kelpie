# Makefile

CXX ?= g++ #clang++
CXXFLAGS = -Iinclude -Wall -std=c++17 -o3 
LDFLAGS =
COMPILER ?= gcc

# Executable name
TARGET = build_$(COMPILER)/matrix_exec

# Source and Object files
SRCS = $(filter-out src/simple_comparison.cpp, $(wildcard src/*.cpp))
OBJS = $(SRCS:src/%.cpp=build_$(COMPILER)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

build_$(COMPILER)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build_clang/* build_gcc/*

.PHONY: all clean
