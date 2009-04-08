#!/bin/bash
# try to run the `main' program and see if any exception happens

for i in `seq 38 54`
do
	echo -n "$i : "
	result=`./main DAC05 $i 2>&1 | grep "solved"`
	if [ -n "$result" ];then
		echo "succeed"
	else
		echo "fail"
	fi
done
