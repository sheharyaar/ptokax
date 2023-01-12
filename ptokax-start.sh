#!/bin/bash

ptokaxId=$(pgrep PtokaX)
if [ -z "$ptokaxId" ]; then
	echo "PtokaX is already running on PID : ${ptokaxId}"
	exit
fi

setsid sudo ~/PtokaX/PtokaX -d -c ~/PtokaX/
echo "Successfully started PtokaX server!"
