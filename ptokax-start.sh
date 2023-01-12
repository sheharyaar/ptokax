#!/bin/bash

GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)

ptokaxId=$(pgrep PtokaX)
if [ -z "$ptokaxId" ]; then
	echo "PtokaX is already running on PID : ${ptokaxId}"
	echo -e "${YELLOW}[-] ${BLUE}PtokaX is already running on PID : ${ptokaxId}${WHITE}"
	exit
fi

setsid sudo ~/PtokaX/PtokaX -d -c ~/PtokaX/
echo -e "${GREEN}[+] ${BLUE}Successfully started PtokaX server!${WHITE}"
