// Filename : main.cpp
// main entrance of the program
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include <iomanip>
#include <iostream>
#include <vector>
#include <time.h>
#include <stdlib.h>
#include "header.h"
#include "GridPoint.h"
#include "Router.h"
#include "main.h"
using std::vector;
using std::endl;
using std::cout;

int main(int argc, char * argv[]){
	Router router;
	router.read_file(argc,argv);
	clock_t start = clock();
	router.solve_cmdline();
	clock_t end = clock();

	FILE * f=fopen("usetime","w");
	fprintf(f,"%ld\n",end-start);
	fclose(f);
	return 0;
}
