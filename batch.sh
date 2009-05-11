#!/bin/bash
# try to run the `main' program and see if any exception happens

if [ $# -lt 1 ];then
	echo "Usage: ./batch benchmark"
	exit 1
fi

FILENAME="$1"
OUTNAME="$FILENAME".time
if [ ! -f "$FILENAME" ];then
	echo "File not exist!"
	exit 1
fi
NUMSUBPROBLEMS=`gawk '/NUMSUBPROBLEMS/ {print $2;}' "$FILENAME"`
if [ -z "$NUMSUBPROBLEMS" ];then
	echo "Error, subproblem num incorrect!"
	exit 1
fi

echo running "$FILENAME", subproblem count = "$NUMSUBPROBLEMS"
echo -n > "$OUTNAME"

for i in `seq 1 "$NUMSUBPROBLEMS"`
do
	echo -n "$i : "
	OUTPUT=`./main "$FILENAME" $i 2>&1 | grep -e "max time" -e "total cell"`
	MAXTIME=`echo "$OUTPUT" | awk '/max time =/ {print $4}'`
	if [ -n "$MAXTIME" ];then
		CELL=`echo "$OUTPUT" | awk '/total cell used/ {print $6}'`
		echo succeed, time = "$MAXTIME", used cell = "$CELL"
		echo "$MAXTIME" "$CELL" >> "$OUTNAME"
	else
		echo fail
	fi
done
