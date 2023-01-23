#!/bin/bash

set -eou pipefail

PYTHON=$(command -v python3 || echo "python3")
PIP=$(command -v pip3 || echo "pip3")
MODULE_1=$($PIP list | grep -cw "netifaces")
MODULE_2=$($PIP list | grep -cw "RPi.GPIO")

for package in "$PYTHON" "$PIP"; do
    if [[ $package != *"/"* ]]; then
        echo "$package package not found. Installing ..."
		sudo apt install -y "$package"
   	fi
done

if [[ $MODULE_1 -eq 0 ]]; then
    echo "Netifaces module not found. Installing ..."
    $PIP install netifaces
fi
if [[ $MODULE_2 -eq 0 ]]; then
    echo "RPi.GPIO module not found. Installing ..."
    $PIP install RPi.GPIO
fi

# Start the script
$PYTHON "/home/pi/ptokax-ip.py"
