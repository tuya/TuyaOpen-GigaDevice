#!/bin/bash

# =============================================================================
# USB Debugger Permissions Setup Script for Linux
# This script configures udev rules to allow non-root access to debuggers.
# Supported: GD-Link, ST-Link, J-Link, CMSIS-DAP
# =============================================================================

# Define the rules content
# 0666 mode allows all users read/write access to the device

#Bus 001 Device 010: ID 1366:0105 SEGGER J-Link
#Bus 001 Device 011: ID 28e9:0797 GDMicroelectronics GDLinker_V2

RULES='
# GigaDevice GD-Link
ATTRS{idVendor}=="28e9", ATTRS{idProduct}=="018a", MODE="0666", GROUP="plugdev", TAG+="uaccess"
ATTRS{idVendor}=="28e9", ATTRS{idProduct}=="0797", MODE="0666", GROUP="plugdev", TAG+="uaccess"

# Segger J-Link
ATTRS{idVendor}=="1366", ATTRS{idProduct}=="0101", MODE="0666", GROUP="plugdev", TAG+="uaccess"
ATTRS{idVendor}=="1366", ATTRS{idProduct}=="0105", MODE="0666", GROUP="plugdev", TAG+="uaccess"

# ST-Link V2 / V2.1 / V3
ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666", GROUP="plugdev", TAG+="uaccess"
ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", GROUP="plugdev", TAG+="uaccess"
ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", MODE="0666", GROUP="plugdev", TAG+="uaccess"

# Generic CMSIS-DAP Debuggers
ATTRS{idVendor}=="d209", ATTRS{idProduct}=="24a1", MODE="0666", GROUP="plugdev", TAG+="uaccess"
ATTRS{idVendor}=="c251", ATTRS{idProduct}=="f001", MODE="0666", GROUP="plugdev", TAG+="uaccess"
'

TARGET_FILE="/etc/udev/rules.d/99-openocd-sdk.rules"

echo "------------------------------------------------"
echo "Setting up USB debugger permissions..."
echo "Note: This operation requires sudo privileges."
echo "------------------------------------------------"

# Check if the rules directory exists
if [ ! -d "/etc/udev/rules.d" ]; then
    echo "Error: /etc/udev/rules.d directory not found. Is this a Linux system?"
    exit 1
fi

# Write rules to the system directory using sudo
echo "$RULES" | sudo tee "$TARGET_FILE" > /dev/null

if [ $? -eq 0 ]; then
    echo "Success: Rules written to $TARGET_FILE"

    # Reload udev rules to apply changes immediately
    echo "Reloading udev rules..."
    sudo udevadm control --reload-rules
    sudo udevadm trigger

    echo "------------------------------------------------"
    echo "Setup complete!"
    echo "IMPORTANT: If your debugger is currently plugged in,"
    echo "please UNPLUG and RE-PLUG it now to apply permissions."
    echo "------------------------------------------------"
else
    echo "Error: Failed to write rules. Please ensure you have sudo access."
    exit 1
fi