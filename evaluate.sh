#!/bin/bash
# column1 = cell used , column2 = run time
if [ "$#" -lt 1 ];then
	echo "Usage: ./evaluate bench_name"
	exit 1
fi

awk 'BEGIN{max=-1} {used+=$1;sum+=$2;if( $2 > max ){max=$2}} END{printf("Max/avg.=%d/%.2f  cell=%d\n",max,sum/NR,used)}' "$1".result
