// Filename : route.cpp
// functions and heuristics used for routing
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include <algorithm>
#include <vector>
#include <deque>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include "header.h"
#include "parser.h"
#include "route.h"
using std::vector;

extern int netcount;
extern Chip chip;
extern int N,M;
extern int netorder[MAXNET];
extern Point path[MAXNET][MAXTIME];
extern BYTE blockage[MAXGRID][MAXGRID];
extern Grid grid[MAXNET][MAXGRID][MAXGRID];
extern int idx;

int dx[]={-1,1,0,0,0};
int dy[]={0,0,1,-1,0};

// perform electrode constraint check
bool electrodeCheck(const Point & pt){
	// use DFS to check : 2-color
	
	return true;
}

// for a net `which' at location `pt' at time `t', 
// perform fluidic constraint check
// if successful return 0 
// else return the conflicting net
int fluidicCheck(int which, const Point & pt,int t){
	int i;
	// for each routed net, 
	// check if the current routing net(which) violate fluidic rule
	// we have known the previous routed net 
	// from netorder[0] to netorder[i]!=which
	// t's range: [1..T]
	for(i=0;i<netcount && netorder[i] != which;i++){
		int checking = netorder[i];
		// static fluidic check
		if( !(abs(pt.x - path[checking][t].x) >=2 ||
		      abs(pt.y - path[checking][t].y) >=2) )
			return i;
		// dynamic fluidic check
		if ( !(abs(pt.x - path[checking][t-1].x) >=2 ||
		       abs(pt.y - path[checking][t-1].y) >=2) )
			return i;
	}
	return 0;
}

// test if a point is in the chip array
bool inGrid(const Point & pt){
	if( pt.x >=0 && pt.x <N && 
	    pt.y >=0 && pt.y <M) return true;
	else return false;
}

// get the neighbour points of a point in the chip
vector<Point> getNbr(const Point & pt){
	vector<Point> s;
	for(int i=0;i<4;i++){
		Point p(pt.x+dx[i],pt.y+dy[i]);
		if( inGrid(p) )
			s.push_back(p);
	}
	return s;
};

// set blockage occupation
void initBlock(Subproblem *p){
	memset(blockage,0,sizeof(blockage));
	int i,x,y;
	for(i=0;i<p->nBlock;i++) {
		Block b = p->block[i];
		for(x=b.pt[0].x;x<=b.pt[1].x;x++)
			for(y=b.pt[0].y;y<=b.pt[1].y;y++)
				blockage[x][y]=1;
	}
}

// swap two integers
void swap(int &a,int &b){
	int t=a;
	a=b;
	b=t;
}

// compare which net should be routed first
/*
int cmpNet(const Net & n1,const Net & n2){
	Block b1,b2;
	b1.pt[LL] = n1.pin[0];
	b1.pt[UR] = n1.pin[1];
	b2.pt[LL] = n2.pin[0];
	b2.pt[UR] = n2.pin[1];
}
*/

// get the bounding box of two nets
Block getBoundingBox(const Pin & p1, const Pin & p2){
	Block bb;
	bb.pt[LL].x = MIN(p1.pt.x,p2.pt.x);
	bb.pt[LL].y = MIN(p1.pt.y,p2.pt.y);
	bb.pt[UR].x = MAX(p1.pt.x,p2.pt.x);
	bb.pt[UR].y = MAX(p1.pt.y,p2.pt.y);
	return bb;
}

// determine if a point is inside a rect
bool ptInRect(const Block & bb, const Point & pt){
	if( pt.x<=bb.pt[UR].x && pt.x>=bb.pt[LL].x &&
	    pt.y<=bb.pt[UR].y && pt.y>=bb.pt[LL].y)
		return true;
	return false;
}

// compare which net should be routed first
int cmpNet(const void * id1, const void * id2){
	int i1 = *(int*)id1;
	int i2 = *(int*)id2;
	Net * n1 = &chip.prob[idx].net[i1];
	Net * n2 = &chip.prob[idx].net[i2];
	Block b1,b2;
	// get two bounding box
	b1 = getBoundingBox(n1->pin[0],n1->pin[1]);
	b2 = getBoundingBox(n2->pin[0],n2->pin[1]);
	// pin1's source inside bounding box 2,should route 1 first
	if( ptInRect(b2,n1->pin[0].pt) ) return -1;
	// pin2's source inside bounding box 1,should route 2 first
	else if( ptInRect(b1,n2->pin[0].pt) ) return 1;

	// use manhattance to judge
	int m1 = MHT(n1->pin[0].pt,n1->pin[1].pt);
	int m2 = MHT(n2->pin[0].pt,n2->pin[1].pt);
	return m1-m2;
}

// use some heuristic to get a net order of a subproblem
// NOW: use manhattance distance & bounding box
//      and only handle 2-pin net by now
// bounded net first, if not bounding happens
// route the short net first
void sortNet(Subproblem * p, int * netorder){
	// do nothing here now, just initialize
	int i;
	int N=p->nNet;
	for(i=0;i<N;i++) netorder[i]=i;
	qsort(netorder,N,sizeof(int),cmpNet);
}

// parse chip description file and store in `chip'
Chip * init(int argc, char * argv[], Chip * chip){
	FILE * f;
	if(argc<3) {
		printf("Usage ./%s filename subproblem\n","main");
		exit(1);
	}
	const char * filename = argv[1];
	idx = atoi(argv[2]);
	//filename = "DAC05";	// temporarily hard code to DAC05
       	if( (f = fopen(filename,"r")) == NULL ){
		printf("open file error\n");
		exit(1);
	}

	parse(f,chip); // now `chip' has store subproblems
	fclose(f);
	return chip;
}

// (heuristic)try to avoid zig-zag , use least bends
Point traceback_line(int which, int t, const Point & current, DIRECTION dir){
	// heuristic: see if along the direction satisfies...
	Point rtn(current.x+dx[dir],current.y+dy[dir]);
	if( inGrid( rtn ) && grid[which][rtn.x][rtn.y] == t-1 )
		return rtn;
	// failed, search other direction
	// note that for the first backtrack step, will reach here
	vector<Point> nbr = getNbr(current); 
	vector<Point>::iterator iter;
	for(iter = nbr.begin();iter!=nbr.end();iter++){
		rtn = (*iter);
		// set to `<=' in case there's stalling
		if( grid[which][rtn.x][rtn.y] <= t-1 ) 
			return rtn;
	}
	fprintf(stderr,"traceback_line failed\n");
	return Point(-1,-1);
}

// determine the relative position of l to r
DIRECTION PtRelativePos(const Point & l ,const Point & r){
	int dx = l.x-r.x;
	int dy = l.y-r.y;
	if( dx < 0 ) return LEFT;
	else if( dx > 0 ) return RIGHT;
	else if( dy < 0 ) return DOWN;
	else if( dy > 0 ) return UP;
	else return STAY;
}

// use some heuristic to trace back a path
void traceback(int which, Point & current){
}

bool checkBending(const Point & p1,const Point & p2){
	if( p1.x == p2.x ||
	    p1.y == p2.y)
		return false;
	return true;
}
