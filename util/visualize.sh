#!/bin/bash
# Automation script to generate visualized result of a droplet routing problem
# 1. use `main' to route the droplet in $FILE
# 2. use `parser' to parse the result
# 3. use pdflatex to generate the result
#
# NOTE: should have two arguments 
# $1=file name 
# $2=sub problem number
# 
# <iveney@gmail.com>

FILE=$1
SUBPROB=$2
MAIN=./main       # use to get the routing result
PARSER=./parser   # use to grep the conrresponding problem and translate to tex file

echo "Start to visualize..."
cat droute_draw_header.tex > "${FILE}.tex"
echo "  solving..."
$MAIN $FILE $SUBPROB |grep -v "Exceed" | ./droute_draw >> "${FILE}.tex"
echo "  parsing..."
$PARSER $FILE $SUBPROB
echo "  greping..."
egrep -e "[\]draw.*pin" -e "[\]blockage" "${FILE}_p${SUBPROB}.tex" >> "${FILE}.tex" 
echo "  finishing..."
cat droute_draw_tail.tex >> "${FILE}.tex"

pdflatex "${FILE}.tex"
