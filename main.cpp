// Filename : main.cpp
// main entrance of the program
//
// Author : Xiao Zigang
// <Thu Mar 25 10:27:39 CST 2010>
// ----------------------------------------------------------------//

#include <iomanip>
#include <string>
#include <iostream>
#include <fstream>
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
using std::ofstream;

int main(int argc, char * argv[]){
	srand(time(NULL));
	Router router;
	router.read_file(argc,argv);
	clock_t start = clock();
	router.solve_cmdline();
	clock_t end = clock();
	clock_t time_used = end-start;

#ifdef DEBUG
	// print the time used in solving the problem
	string filename(argv[1]);
	filename+=".usetime";
	ofstream fout(filename.c_str());
	fout<<time_used;
	fout.close();
#endif
	return 0;
}
