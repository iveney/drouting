// Filename : main.cpp
// main entrance of the program
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include <iomanip>
#include <vector>
#include "header.h"
#include "GridPoint.h"
#include "Router.h"
#include "main.h"
using std::vector;

int main(int argc, char * argv[]){
	Router router;
	router.read_file(argc,argv);
	vector<RouteResult> result = router.solve_cmdline();
	return 0;
}
