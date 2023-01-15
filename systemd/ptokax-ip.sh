#!/bin/bash

set -eou pipefail

PYTHON=$(command -v python3 || echo "python3")
PIP=$(command -v pip3 || echo "pip3")
MODULE=$($PIP list | grep -cw "netifaces")

for package in "$PYTHON" "$PIP"; do
    if [[ $package != *"/"* ]]; then
        echo "$package package not found. Installing ..."
		sudo apt install -y "$package"
   	fi
done

if [[ $MODULE -eq 0 ]]; then
    echo "Netifaces module not found. Installing ..."
    $PIP install netifaces
fi

# Start the script
$PYTHON "/home/pi/ptokax-ip.py"
