// Filename : main.cpp
// main entrance of the program
//
// Author : Xiao Zigang
// <Thu Mar 25 10:27:39 CST 2010>
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
	srand(time(NULL));
	Router router;
	router.read_file(argc,argv);
	clock_t start = clock();
	router.solve_cmdline();
	clock_t end = clock();

	// print the time used in solving the problem
#ifdef DEBUG
	FILE * f=fopen("usetime","w");
	fprintf(f,"%ld\n",end-start);
	fclose(f);
#endif
	return 0;
}
