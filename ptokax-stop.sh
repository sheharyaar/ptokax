#!/bin/bash

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)

ptokaxId=$(pgrep PtokaX)
if [ -z "$ptokaxId" ]; then
	echo -e "${YELLOW}[-] ${BLUE}PtokaX is not running currently${WHITE}"
	exit
fi

sudo kill -SIGTERM "$ptokaxId"
echo -e "${GREEN}[+] ${BLUE}Successfully stopped PtokaX server at PID${WHITE}[${RED}${ptokaxId}${WHITE}]${BLUE}!${WHITE}"
