#!/bin/bash
if [ "$#" -lt 1 ];then
	echo "Usage: ./evaluate runtime_file"
	exit 1
fi

awk '{sum+=$1} END{print sum/NR}' "$1".time
