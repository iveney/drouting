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
# bugs:
# (color) :
#	  netorder is specified in program but the parse result do not consider the route order
#         hence in the output pdf file, some net's src and dst do not have the same color
#	  as their route
#
# (solved):
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
OUTPUT="${FILE}_r${SUBPROB}.tex"
MAIN=./main       # use to get the routing result
PARSER=./parser   # use to parse the conrresponding problem and convert to tex

###########################################################################
echo "Start to visualize..."
cat droute_draw_header.tex > "${OUTPUT}"

# get the route result first
echo "  solving..."
ROUTERESULT=`$MAIN $FILE $SUBPROB`

# extract information needed by drawing util
echo "  parsing..."
ROUTE=`echo "$ROUTERESULT" | tail -n +6 | \
	grep -v -e "Exceed" -e "pin" -e "\*" -e "Find"`
echo "$ROUTE" | ./droute_draw >> ${OUTPUT} 
$PARSER $FILE $SUBPROB

echo "  greping..."
INTERMEDIATE=${FILE}_p${SUBPROB}.tex
egrep -e "[\]draw.*pin" -e "[\]blockage" "$INTERMEDIATE" >> "${OUTPUT}" 

echo "  finishing..."
cat droute_draw_tail.tex >> "${OUTPUT}"
exit 1

pdflatex "${OUTPUT}"

# do some clean up stuff
# remove the intermediate result
rm $INTERMEDIATE
OTHER=${OUTPUT%.tex}
for x in  log out aux
do
	rm ${OTHER}.$x
done

# output file name to a log file so that we can check it fast
echo "${OTHER}.pdf" > log

# just see it
./see
