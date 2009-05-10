#!/bin/bash
if [ "$#" -lt 1 ];then
	echo "Usage: ./evaluate runtime_file"
	exit 1
fi

awk '{sum+=$1;used+=$2} END{print "avg time=" sum/NR, "cell=" used}' "$1".time
