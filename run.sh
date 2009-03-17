#!/bin/bash

for i in `seq 1 78`
do
	result=`./main DATE06_3 $i 2>&1 | grep Exceed`
	if [ -n "$result" ];then
		echo "$i : $result"
	fi
done
