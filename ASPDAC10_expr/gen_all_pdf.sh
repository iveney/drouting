#!/bin/bash

DIRS=`ls -d DA*`
for i in $DIRS
do
	cd $i
	for j in `ls "$i"*_sol.tex`
	do
		echo "processing $j..."
		pdflatex "$j" >/dev/null
	done
	rm *.out  *.log *.aux
	cd ..
done
