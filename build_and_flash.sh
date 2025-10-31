#!/bin/bash

# ESP32 MDP Firmware Build and Flash Script
# =========================================
#
# This script builds and flashes the ESP32 MDP firmware for testing
# L3↔L4 communication over WiFi.
#
# Prerequisites:
# - Arduino IDE with ESP32 board support
# - ESP32 connected via USB
# - WiFi credentials configured in the .ino file
#
# Usage:
#   ./build_and_flash.sh
#

set -e  # Exit on any error

echo "=== ESP32 MDP Firmware Builder ==="
echo ""

# Configuration
ESP32_PORT="/dev/cu.usbserial-*"  # Adjust if different
ESP32_BOARD="esp32:esp32:esp32"
BUILD_DIR="./build"

# Check if Arduino CLI is available
if ! command -v arduino-cli &> /dev/null; then
    echo "❌ arduino-cli not found!"
    echo "Please install Arduino CLI:"
    echo "  brew install arduino-cli"
    echo "  arduino-cli config init"
    echo "  arduino-cli core install esp32:esp32"
    exit 1
fi

# Check if ESP32 core is installed
if ! arduino-cli core list | grep -q "esp32:esp32"; then
    echo "📦 Installing ESP32 core..."
    arduino-cli core install esp32:esp32
fi

# Find ESP32 serial port
ESP32_PORT=$(ls /dev/cu.usbserial-* 2>/dev/null | head -1 || echo "")
if [ -z "$ESP32_PORT" ]; then
    echo "❌ ESP32 not found on USB!"
    echo "Please connect your ESP32 and check:"
    echo "  ls /dev/cu.usbserial-*"
    echo ""
    echo "Available ports:"
    ls /dev/cu.* 2>/dev/null || echo "No serial ports found"
    exit 1
fi

echo "✅ ESP32 found on: $ESP32_PORT"
echo "✅ Using board: $ESP32_BOARD"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"

# Build the firmware
echo "🔨 Building ESP32 firmware..."
arduino-cli compile \
    --fqbn "$ESP32_BOARD" \
    --build-path "$BUILD_DIR" \
    --verbose \
    esp32_mdp_l3.ino

echo "✅ Build completed!"
echo ""

# Flash the firmware
echo "⚡ Flashing ESP32..."
arduino-cli upload \
    --fqbn "$ESP32_BOARD" \
    --port "$ESP32_PORT" \
    --verbose \
    esp32_mdp_l3.ino

echo "✅ Flash completed!"
echo ""

# Monitor serial output
echo "📡 Starting serial monitor..."
echo "Press Ctrl+C to exit"
echo ""
arduino-cli monitor \
    --port "$ESP32_PORT" \
    --config baudrate=115200

echo ""
echo "=== ESP32 MDP Firmware Ready ==="
echo "Your ESP32 should now be:"
echo "1. Connected to WiFi"
echo "2. Running UDP server on port 4040"
echo "3. Ready to receive MDP packets from M1 Mac"
echo ""
echo "Next: Run the UDP demo on your M1:"
echo "  cd /Users/josephrost/moop-may/src/l0_riscv_esp32_layered"
echo "  make demo"
