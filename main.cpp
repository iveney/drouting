// Filename : main.cpp
// main entrance of the program
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include <iomanip>
#include <iostream>
#include <vector>
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
	router.solve_cmdline();
	cout<<"max time = "<<router.max_t<<" "<<endl;
	return 0;
}
