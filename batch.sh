#!/bin/bash
# try to run the `main' program and see if any exception happens

PROBLEMNAME=DAC05_2
for i in `seq 1 78`
do
	echo -n "$i : "
	result=`./main $PROBLEMNAME $i 2>&1 | grep "solved"`
	if [ -n "$result" ];then
		echo "succeed"
	else
		echo "fail"
	fi
done
