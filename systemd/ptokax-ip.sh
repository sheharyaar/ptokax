#!/bin/bash

PYTHON=$(which python3)
if [[ "x$PYTHON" == "x" ]]; then
    # Install python raspberry pi
    echo "Python3 not found. Installing ..."
fi

echo "Python found at : $PYTHON"

PIP=$(which pip3)
if [[ "x$PIP" == "x" ]]; then
    # Install pip raspberry pi
    echo "Pip not found. Installing ..."
fi

echo "Pip found at : $PIP"

MODULE=$(pip list | grep -cw "netifaces")
if [[ $MODULE -eq 0 ]]; then
    echo "Netifaces module not found. Installing ..."
    $PIP install netifaces
fi

# Start the script
$PYTHON "/home/pi/ptokax-ip.py"