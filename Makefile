CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -pedantic -O2 -Iinclude
TARGET   := erl_hub
SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(SOURCES:src/%.cpp=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp $(wildcard include/*.h) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET) *.bin *.tmp

.PHONY: all run clean
