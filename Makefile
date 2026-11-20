# SPDX-License-Identifier: GPL-3.0-only
WINCXX ?= i686-w64-mingw32-g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Werror

.PHONY: all test
all: build/TTWVRNightVision.dll

build/TTWVRNightVision.dll: src/Plugin.cpp src/Gesture.h | build
	$(WINCXX) $(CXXFLAGS) -shared -static -s $< -o $@

build/GestureTest: tests/Gesture.cpp src/Gesture.h | build
	$(CXX) $(CXXFLAGS) $< -o $@

build:
	mkdir -p $@

test: build/GestureTest
	./build/GestureTest
