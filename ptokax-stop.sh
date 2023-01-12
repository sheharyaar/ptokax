#!/bin/bash

ptokaxId=$(pgrep PtokaX)
if [ -z "$ptokaxId" ]; then
	echo "PtokaX is not running currently"
	exit
fi

echo "PID : ${ptokaxId}"

sudo kill -SIGTERM "$ptokaxId"

echo "Successfully stopped PtokaX!"
