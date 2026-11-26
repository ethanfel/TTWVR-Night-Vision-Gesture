# SPDX-License-Identifier: GPL-3.0-only
WINCXX ?= i686-w64-mingw32-g++
HOST_VERSION := 1.1.8
REVISION := 1
VERSION := $(HOST_VERSION)-r$(REVISION)
# xNVSE exposes a single integer. Use the same decimal fields as the other TTWVR addons.
PLUGIN_VERSION := $(shell printf '%d%02d%02d%02d' $(subst ., ,$(HOST_VERSION)) $(REVISION))
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Werror

.PHONY: all test package
all: build/TTWVRNightVision.dll

build/TTWVRNightVision.dll: src/Plugin.cpp src/Gesture.h Makefile | build
	$(WINCXX) $(CXXFLAGS) -DNVG_VERSION='"$(VERSION)"' -DNVG_PLUGIN_VERSION=$(PLUGIN_VERSION) -shared -static -s $< -o $@

build/GestureTest: tests/Gesture.cpp src/Gesture.h | build
	$(CXX) $(CXXFLAGS) $< -o $@

build:
	mkdir -p $@

test: build/GestureTest
	./build/GestureTest

package: all test
	python3 tools/package.py $(VERSION)
