#!/bin/bash

PYTHON=$(command -v python3 || echo "python3")
PIP=$(command -v pip3 || echo "pip3")

for package in "$PYTHON" "$PIP"; do
    if [[ $package != *"/"* ]]; then
        echo "$package package not found. Installing ..."
		sudo apt install -y "$package"
   	fi
done

for MODULE in "netifaces" "RPi.GPIO"; do
	MODULE_INSTALLED=$($PIP list | grep -cw "$MODULE")
	if [[ $MODULE_INSTALLED -eq 0 ]]; then
		echo "$MODULE module not found. Installing ..."
		$PIP install "$MODULE"
	fi
done

# Summon the python script 
$PYTHON "/home/pi/MetaHub/psm/systemd/ptokax-network-check.py"

# Start ptokax finally
/home/pi/MetaHub/PtokaX/PtokaX -c /home/pi/MetaHub/PtokaX/
