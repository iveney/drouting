#!/bin/bash

for i in `seq 1 20`
do
	./main placement/DAC05.plc > DAC05.$i.result 2>&1
done
