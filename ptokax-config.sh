#!/bin/bash

GREEN=$(tput setaf 2)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)

echo -e "${GREEN}[+] ${BLUE}Configuring PtokaX ...${WHITE}"
~/PtokaX/PtokaX -m
