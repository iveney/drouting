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
		     pin distance = 1.2cm*\\scale, \n \
		     pins/.style={rectangle,draw,fill=brown,font=\\scriptsize}, \n \
		     pinsnofill/.style={rectangle,draw,font=\\scriptsize}, \n \
		     arrow/.style={->,very thick}, \n \
		     block/.style={gray}]\n"); 
}
void end_figure(FILE * fig,int time){
	fprintf(fig,"\\end{tikzpicture} \n \
		     \\caption{time=%d} \n \
	     	     \\end{figure} \n \
		     \\clearpage\n",time);
}

void draw_voltage(const RouteResult & result){
	// fill in template header
	const char * fn = "result.tex";
	char cmd[100];
       	sprintf(cmd,"cat result_header.tex > %s",fn);
	system(cmd);

	// ************************************
	// main body
	FILE * fig = fopen("result.tex","a");
	int i,j,t;
	for(t=0;t<=result.T;t++){
		begin_figure(fig);
		// draw each droplet
		for (i = 0; i < result.pProb->nNet; i++) {
			const NetRoute & net_path = result.path[i];
			for (j = 0; j < net_path.num_pin-1; j++) {
				Point pt = net_path.pin_route[j][t];
				// temporarily use red color
				fprintf(fig,"\\node[pins,fill=%s] (net_%d_%d) at (%d+\\half,%d+\\half) {\\tt %d};\n",
						"red",i,j,pt.x,pt.y,t);
			}
		}
		// draw grids
		fprintf(fig,"\\drawgrid{%d}{%d}]\n",result.W,result.H);

		// draw the voltages

		// end time frame
		end_figure(fig,t);
	}

	// ************************************
	fprintf(fig,"\\end{document}");
	fclose(fig);
	// fill in template footer
}
