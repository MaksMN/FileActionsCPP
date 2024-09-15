CPP := $(subst ./,,$(shell find -name '*.cpp')) # All cpp files in this folder recursive

SRC = $(CPP)
DEBUG_TARGET = bin/debug/out
RELEASE_TARGET = bin/release/out

default:
debug:
	mkdir -p bin/debug
	g++ -fdiagnostics-color=always -std=c++20 -g $(SRC) -o $(DEBUG_TARGET)

release:
	mkdir -p bin/release
	g++ -std=c++20 $(SRC) -o $(RELEASE_TARGET)