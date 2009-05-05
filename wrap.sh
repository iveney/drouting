#!/bin/bash
./main $1 $2
OUTPUT="$1"_"$2"_sol
echo "pdflatex ... "
pdflatex "$OUTPUT".tex > /dev/null
echo "...finish, starting acroread..."
acroread "$OUTPUT".pdf
