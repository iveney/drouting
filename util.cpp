// ----------------------------------------------------------------//
// Filename : util.cpp
// Source code for some utility functions
//
// Author : Xiao Zigang
// Modifed: <Fri Feb 13 16:46:54 HKT 2009> 
// ----------------------------------------------------------------//

#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "header.h"
#include "util.h"
using std::vector;

// report an error string and exit the program
// Note that it is wrapped by a macro `report_exit'
// Do NOT use this directly.
void _report_exit(const char *location, const char *msg){
	  fprintf(stderr,"Error at %s: %s\n", location, msg);
	  exit(1);
}

bool check_bending(const Point & p1,const Point & p2){
	if( p1.x == p2.x ||
	    p1.y == p2.y)
		return false;
	return true;
}

// determine the relative position from l to r
// NOTE: l is supposed to be adjacent to r
// hence only four possible case
DIRECTION pt_relative_pos(const Point & l ,const Point & r){
	int dx = l.x-r.x;
	int dy = l.y-r.y;
	if( dx < 0 ) return LEFT;
	else if( dx > 0 ) return RIGHT;
	else if( dy < 0 ) return DOWN;
	else if( dy > 0 ) return UP;
	else return STAY;
}
