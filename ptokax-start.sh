#!/bin/bash

ptokaxId=$(pgrep PtokaX)
if [ "$ptokaxId" != "" ]; then
	echo "PtokaX is already running on PID : ${ptokaxId}"
	exit
fi

sudo ~/PtokaX/PtokaX -d -c ~/PtokaX/ &
echo "Successfully started PtokaX server!"
exit
