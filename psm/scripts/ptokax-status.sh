#!/bin/bash

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
YELLOW=$(tput setaf 3)
BLUE=$(tput setaf 4)
WHITE=$(tput setaf 7)

ptokaxId=$(pgrep PtokaX)
if [ -n "$ptokaxId" ]; then
	echo -e "${GREEN}  [ACTIVE]${YELLOW} : PID : ${BLUE}${ptokaxId}${WHITE}"
else
	echo -e "${RED}  [INACTIVE]${WHITE}"
fi
