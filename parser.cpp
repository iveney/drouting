// ----------------------------------------------------------------//
// Filename : parser.cpp
// source code to Parse the configuration (problem description)
// and generate the tex file
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include "parser.h"

// enumerate color from a static color string table
const char * getColor(){
	static int counter=0;
	const static char * colors[]={
		"blue","green","orange",
		"purple","yellow","pink",
		"violet","red","cyan"};
	const char * p=colors[counter];
	int size=sizeof(colors)/sizeof(char*);
	counter=(counter+1)%size;
	return p;
}

// given a file pointer to a configuration file
// parse it and store the droplet routing subproblems to chip
// 1. `#' is not handled
// 2. all strings must be exactly the same
Chip * parse(FILE * f,Chip * chip){
	//char buf[MAXBUF];
	fscanf(f,"ARRAY: %d %d\n",&chip->W,&chip->H);
	fscanf(f,"TIME: %d\n",&chip->time);
	fscanf(f,"TIMECONSTRAINT: %d\n",&chip->T);
	fscanf(f,"WAT: %d %d\n",&(chip->WAT.x),&(chip->WAT.y));
	fscanf(f,"NUMSUBPROBLEMS: %d\n",&chip->nSubProblem);
	int i,j,k,temp;
	// scan for each subproblem, index start from 1
	for(i=1;i<=chip->nSubProblem;i++){
		Subproblem * pSub = &chip->prob[i];
		fscanf(f,"BEGIN SUBPROBLEM %d\n",&temp);
		fscanf(f,"NUMBLOCKS: %d\n", &pSub->nBlock);
		Block *pBlk = pSub->block;
		// scan for each block
		for(j=0;j<pSub->nBlock;j++){
			fscanf(f,"BLOCK %s\n",pBlk[j].name);
			fscanf(f,"%d %d %d %d\n",
					&pBlk[j].pt[0].x,
					&pBlk[j].pt[0].y,
					&pBlk[j].pt[1].x,
					&pBlk[j].pt[1].y);
		}// end for block
		fscanf(f,"NUMNETS: %d\n",&pSub->nNet);
		Net * pNet = pSub->net;
		// scan for each net
		for(j=0;j<pSub->nNet;j++){
			fscanf(f,"NET %s\n",pNet[j].name);
			fscanf(f,"NUMPINS: %d\n",&pNet[j].numPin);
			Pin *p = pNet[j].pin;
			// scan for each pin
			for(k=0;k<pNet[j].numPin;k++){
				fscanf(f,"PIN %s\n",p[k].name);
				fscanf(f,"%d %d\n",
						&p[k].pt.x,
						&p[k].pt.y);
			}// end of pin
		}// end of net
		fscanf(f,"END SUBPROBLEM %d\n",&temp);
	}// end of subproblem
	return chip;
}

// use tgf/tikz to draw A subproblem
// can be used with ``gen.sh'' to generate all subproblems
void drawSubproblem(Subproblem * prob, int W,int H,int num,char * name){
	char figName[MAXBUF];
	char filename[MAXBUF];
	char cmd[MAXBUF];
	sprintf(filename,"%s_p%d.tex",name,num);
	// dump a template to the target file
	sprintf(cmd,"cp template.tex %s",filename);	
	// note that if the file name contains `_', 
	// should manually replace it, since it is a special charater in LaTeX
	sprintf(figName,"%s\\_subproblem\\_%d",name,num);
	// output tex head from template
	system(cmd);

	FILE * fig = fopen(filename,"a");
	int i;

	// define \W and \H
	fprintf(fig,"%% define the row and column number\n");
	fprintf(fig,"\\def \\W{%d}  ",W);
	fprintf(fig,"\\def \\H{%d}\n",H);

	// draw block
	fprintf(fig,"%% blockages\n");
	Block * pBlk = prob->block;
	for(i=0;i<prob->nBlock;i++){
		fprintf(fig,"\\blockage{%d}{%d}{%d}{%d}\n",
				pBlk[i].pt[0].x,
				pBlk[i].pt[0].y,
				pBlk[i].pt[1].x,
				pBlk[i].pt[1].y);
	}

	// draw net
	fprintf(fig,"%% nets\n");
	Net * pNet = prob->net;
	for(i=0;i<prob->nNet;i++){
		if(pNet[i].numPin==2){
			fprintf(fig,
			"\\drawtwopin{%s}{%d}{%d}{%s}{%d}{%d}{%s!70}\n",
			pNet[i].pin[0].name,
			pNet[i].pin[0].pt.x,
			pNet[i].pin[0].pt.y,
			pNet[i].pin[1].name,
			pNet[i].pin[1].pt.x,
			pNet[i].pin[1].pt.y,
			getColor());
		}
		else{// 3 pin net
			fprintf(fig,
			"\\drawthreepin{%s}{%d}{%d}{%s}{%d}{%d}{%s}{%d}{%d}\n",
			pNet[i].pin[0].name,
			pNet[i].pin[0].pt.x,
			pNet[i].pin[0].pt.y,
			pNet[i].pin[1].name,
			pNet[i].pin[1].pt.x,
			pNet[i].pin[1].pt.y,
			pNet[i].pin[2].name,
			pNet[i].pin[2].pt.x,
			pNet[i].pin[2].pt.y);
		}
	}

	// draw grids
	fprintf(fig,"\\drawgrid{\\W}{\\H}\n");

	// output remaing part
	fprintf(fig,"\\end{tikzpicture}\n");
	fprintf(fig,"\\caption{%s}\n",figName);
	fprintf(fig,"\\end{figure}\n\\clearpage\n");
	fclose(fig);
	// after draw, format the file
}

