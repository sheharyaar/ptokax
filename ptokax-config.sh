#!/bin/bash

GREEN=$(tput setaf 2)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)

echo -e "${GREEN}[+] ${BLUE}Configuring PtokaX ...${WHITE}"
cd ~/MetaHub/PtokaX/ || (echo "cd to PtokaX failed" && exit)
./PtokaX -m
cd ~ || (echo "cd to ~ failed" && exit)
