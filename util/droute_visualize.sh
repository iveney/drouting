#!/bin/bash
# should have two arguments 
# $1=file name $2=sub problem number

FILE=$1
SUBPROB=$2

echo "Start to visualize..."
cat droute_draw_header.tex > "${FILE}.tex"
echo "  solving..."
./main $FILE $SUBPROB |grep -v "Exceed" | ./droute_draw >> "${FILE}.tex"
echo "  parsing..."
./parser $FILE $SUBPROB
echo "  greping..."
egrep -e "[\]draw.*pin" -e "[\]blockage" "${FILE}_p${SUBPROB}.tex" >> "${FILE}.tex" 
echo "  finishing..."
cat droute_draw_tail.tex >> "${FILE}.tex"
