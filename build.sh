#!/bin/bash

echo "
    _         _ _   _     ___  ___
   / \   _ __(_) |_| |__ / _ \/ __|
  / _ \ | '__| | __| '_ \ (_) \__ \\
 / ___ \| |  | | |_| | | \__, |___/
/_/   \_\_|  |_|\__|_| |_| /_/
                                   
 Pentesting Calculator Firmware
"

if [ -z "$PICO_SDK_PATH" ]; then
    echo "Error: PICO_SDK_PATH not set"
    echo "export PICO_SDK_PATH=/path/to/pico-sdk"
    exit 1
fi

if [ ! -d "$PICO_SDK_PATH" ]; then
    echo "Error: SDK not found at $PICO_SDK_PATH"
    exit 1
fi

mkdir -p build
cd build

echo "Running CMake..."
cmake .. || { echo "CMake failed"; exit 1; }

echo "Building..."
make -j4 || { echo "Build failed"; exit 1; }

echo ""
echo "Build successful!"
echo "Firmware: build/arithmos.uf2"
echo ""
echo "To flash:"
echo "1. Connect Pico while holding BOOTSEL"
echo "2. Copy arithmos.uf2 to RPI-RP2 drive"