// ----------------------------------------------------------------//
// Filename : parser_main.cpp
// Parse the configuration (problem description)
// and generate a tex file
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//
#include "parser.h"
#include "main.h"

int main(int argc, char * argv[]){
	FILE * f;
	Chip chip;
	int idx=1;

	if(argc<2) {
		printf("Usage ./draw filename [subproblem]");
		return 1;
	}
       	if( (f = fopen(argv[1],"r")) == NULL ){
		printf("open file error\n");
		return 1;
	}

	parse(f,&chip);

	if(argc>2){// draw a specific subproblem
		idx=atoi(argv[2]);
		if(idx>chip.nSubProblem) {
			printf("subproblem index out of bound!\n");
			return 1;
		}
		drawSubproblem(&chip.prob[idx],chip.W,chip.H,idx,argv[1]);
	}
	else{// draw all subproblems
		int i=1;
		for(i=1;i<=chip.nSubProblem;i++){
			drawSubproblem(&chip.prob[i],chip.W,chip.H,i,argv[1]);
		}
	}

	fclose(f);
	return 0;
}
