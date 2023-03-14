#!/bin/bash

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)

ptokaxId=$(pgrep PtokaX)
if [ -n "$ptokaxId" ]; then
	echo -e "${YELLOW}[-] ${BLUE}PtokaX is already running on PID : ${RED}${ptokaxId}${WHITE}"
	exit
fi

setsid sudo ~/MetaHub/PtokaX/PtokaX -d -c ~/MetaHub/PtokaX/
echo -e "${GREEN}[+] ${BLUE}Successfully started PtokaX server!${WHITE}"
