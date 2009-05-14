#!/bin/bash
# column1 = cell used , column2 = run time
if [ "$#" -lt 1 ];then
	echo "Usage: ./evaluate bench_name"
	exit 1
fi

awk '{sum+=$2;used+=$1} END{print "avg time=" sum/NR, "cell=" used}' "$1".result
