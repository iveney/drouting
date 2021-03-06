#include <stdio.h>
#include "draw_voltage.h"

// very nasty
void begin_figure(FILE * fig){
	fprintf(fig,"\\begin{figure} \n \
		     \\centering \n \
		     \\begin{tikzpicture}[scale=\\scale, \n \
		     inner sep=0, \n \
		     minimum size=1cm*\\scale, \n \
		     >=latex, \n \
		     pin distance = 1.2cm*\\scale]\n"); 
	  //,font=\\scriptsize
}
void end_figure(FILE * fig,int time){
	fprintf(fig,"\\end{tikzpicture} \n \
		     \\caption{time=%d} \n \
	     	     \\end{figure} \n \
		     \\clearpage\n",time);
}

void draw_voltage(const RouteResult & result,const Chip & chip,
		const char *filename){
	// fill in template header
	// const char * fn = filename.c_str();
#ifdef DEBUG
	printf("output to %s\n",filename);
#endif
	char cmd[MAXBUF];
       	sprintf(cmd,"cat result_header.tex > %s",filename);
	system(cmd);

	// ************************************
	// main body
	FILE * fig = fopen(filename,"a");
	fprintf(fig,"\\def \\W{%d}",result.W);
	fprintf(fig,"\\def \\H{%d}\n",result.H);
	int i,j,t;
	for(t=0;t<=result.T;t++){
		begin_figure(fig);

		// draw the voltages
		for (i = 0; i < result.H; i++) {
			if( result.v_row[t][i] == HI )
				fprintf(fig,"\\drawhline{%d}{%d}\n",i,1);
			else if(result.v_row[t][i] == LO )
				fprintf(fig,"\\drawhline{%d}{%d}\n",i,2);
		}

		for (i = 0; i < result.W; i++) {
			if( result.v_col[t][i] == HI )
				fprintf(fig,"\\drawvline{%d}{%d}\n",i,1);
			else if(result.v_col[t][i] == LO )
				fprintf(fig,"\\drawvline{%d}{%d}\n",i,2);
		}
		// draw activated cells
		const char * fmtstr_cell = "\\node[pins,fill=%s] () at \
					    (%d+\\half,%d+\\half) {} ;\n";
		for(size_t k=0;k<result.activated[t].size();k++){
			fprintf(fig,fmtstr_cell, "green",
				result.activated[t][k].x,
				result.activated[t][k].y);
		}

		// draw block
		Block * pBlk = result.pProb->block;
		for(i=0;i<result.pProb->nBlock;i++){
			fprintf(fig,"\\blockage{%d}{%d}{%d}{%d}\n",
					pBlk[i].pt[0].x,
					pBlk[i].pt[0].y,
					pBlk[i].pt[1].x,
					pBlk[i].pt[1].y);
		}

		const char * fmtstr_2pin = "\\node[pins,fill=%s] (net_%d_%d) \
				    at (%d+\\half,%d+\\half) {\\tt %d};\n";
		const char * fmtstr_3pin = "\\node[pins,fill=%s] (net_%d_%d) \
					    at (%d+\\half,%d+\\half) \
				    {$\\mathtt{%d}$\\_$\\mathtt{%d}$};\n";

		// draw each droplet
		for (i = 0; i < result.pProb->nNet; i++) {
			const NetRoute & net_path = result.path[i];
			for (j = 0; j < net_path.num_pin-1; j++) {
				// for those routed to waste disposal point
				if( t >= (int)net_path.pin_route[j].size() ) 
					break;
				Point pt = net_path.pin_route[j][t];
				// temporarily use red color
				if( net_path.num_pin == 2 )
					fprintf(fig,fmtstr_2pin,
						"red",i,j,pt.x,pt.y,i);
				else{
					if( t<net_path.merge_time )
						fprintf(fig,fmtstr_3pin,"red",
							i,j,pt.x,pt.y,i,j);
					else
						fprintf(fig,fmtstr_2pin,"red",
							i,j,pt.x,pt.y,i,j);
				}
			}
		}

		// draw grids
		fprintf(fig,"\\drawgrid{%d}{%d}]\n",result.W,result.H);
		fprintf(fig,"\\drawWAT{%d}{%d}\n",chip.WAT.x,chip.WAT.y);

		// end time frame
		end_figure(fig,t);
	}

	// ************************************
	fprintf(fig,"\\end{document}");
	fclose(fig);
	// fill in template footer
}
