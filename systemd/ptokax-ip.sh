#!/bin/bash

PYTHON=$(which python3 || echo "python3")
PIP=$(which pip3 || echo "pip3")
MODULE=$(pip list | grep -cw "netifaces")

for package in "$PYTHON" "$PIP"; do
    if [[ $package != *"/"* ]]; then
        echo "$cmd package not found. Installing ..."
    	# install package
        # sudo apt install "$package" # will this not work.. idk what happens when system boots 
   	fi
done

if [[ $MODULE -eq 0 ]]; then
    echo "Netifaces module not found. Installing ..."
    $PIP install netifaces
fi

# Start the script
$PYTHON "/home/pi/ptokax-ip.py"
