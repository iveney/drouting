#!/bin/bash
# Automation script to generate visualized result of a droplet routing problem
# 1. use `main' to route the droplet in $FILE
# 2. use `parser' to parse the result
# 3. use pdflatex to generate the result
# 4. use droute_draw to convert routing result to tex file
#
# NOTE: should have two arguments 
# $1=file name 
# $2=sub problem number
# 
# bug(solved):
#	  It's strange that pdflatex can not generate the file
#	  But CTeX can (also uses pdflatex)
# solution:
#	  ubuntu 8.04(hardy) has pgf 1.x while the custom style specified in 
#	  tikz environment only available in 2.x
#	  download deb package and install solves the problem
#
# Last modified : 2009年 04月 05日 星期日 17:28:48 CST
# <iveney@gmail.com>

if [ "$#" -lt "2" ];then
	echo "Usage: ./visualize.sh filename subproblem_num"
	exit 1
fi

FILE=$1
SUBPROB=$2
MAIN=./main       # use to get the routing result
PARSER=./parser   # use to grep the conrresponding problem and translate to tex file

echo "Start to visualize..."
cat droute_draw_header.tex > "${FILE}.tex"
echo "  solving..."
# get the route result first
ROUTERESULT=`$MAIN $FILE $SUBPROB`
# extract information needed by drawing util
ROUTE=`echo "$ROUTERESULT" | tail -n +6 | grep -v -e "Exceed" -e "pin" -e "\*" -e "Find"`
echo "$ROUTE" | ./droute_draw >> "${FILE}.tex"
echo "  parsing..."
$PARSER $FILE $SUBPROB
echo "  greping..."
egrep -e "[\]draw.*pin" -e "[\]blockage" "${FILE}_p${SUBPROB}.tex" >> "${FILE}.tex" 
echo "  finishing..."
cat droute_draw_tail.tex >> "${FILE}.tex"

pdflatex "${FILE}.tex"
