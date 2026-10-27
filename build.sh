#!/usr/bin/env bash
set -e
mkdir -p build
FLAGS="-std=c++17 -Wall -Wextra -O2 -g -Iinclude"
g++ $FLAGS src/main.cpp src/frame.cpp src/switch.cpp src/lpm.cpp -o build/miniswitch
g++ $FLAGS tests/test_switch.cpp src/frame.cpp src/switch.cpp src/lpm.cpp -o build/tests
echo "Build OK"