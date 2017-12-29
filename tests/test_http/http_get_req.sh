#!/bin/bash

# $1: IP
# $2: port
# $3: uri
# $4: payload

if [ $# -lt 4 ]; then
	echo "Usage: $0 ip port uri payload"
	exit 0
fi

curl --http1.1 -X GET -H "Accept: version1" -H "Content-Type: json" -d "$4" http://${1}:${2}${3} 

