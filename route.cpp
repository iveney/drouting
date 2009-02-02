#include <algorithm>
#include <vector>
//#include <set>
#include <deque>
#include <iostream>
#include <iomanip>
#include "header.h"
#include "parser.h"
using namespace std;

enum DIRECTION{LEFT,RIGHT,UP,DOWN,STAY}; // LEFT, RIGHT, UP, DOWN
static int dx[]={-1,1,0,0,0};
static int dy[]={0,0,1,-1,0};

BYTE blockage[MAXGRID][MAXGRID];	// Blockage bitmap
Chip chip;				// Chip data and subproblem
Grid grid[MAXNET][MAXGRID][MAXGRID];	// record the routes
int N,M;				// row/column count
int netorder[MAXNET];			// net routing order
int netcount;				// current subproblem's net count
Point path[MAXNET][MAXTIME];		// the routing path of each net

// perform electrode constraint check
bool electrodeCheck(const Point & pt){
	return true;
}

// perform fluidic constraint check
bool fluidicCheck(int which, const Point & pt){
	int i;
	// for each routed net, 
	// check if the current routing net violate any rule
	for(i=0;i<netcount && netorder[i] != which;i++){
		int checking = netorder[i];
		// static fluidic check
		if( grid[checking][pt.x][pt.y] )
			return false;
		// dynamic fluidic check
		else if ( grid[checking][pt.x][pt.y] )
			return false;
	}
	return true;
}

// test if a point is in the chip array
bool inGrid(const Point & pt){
	if( pt.x >=0 && pt.x <=N && 
	    pt.y >=0 && pt.y <=M) return true;
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

// use some heuristic to get a net order of a subproblem
void sortNet(Subproblem * p, int * netorder){
	// do nothing here now, just initialize
	int i;
	for(i=0;i<p->nNet;i++)
		netorder[i]=i;
}

// parse chip description file and store in `chip'
Chip * init(int argc, char * argv[], Chip * chip){
	FILE * f;
	/*
	if(argc<2) {
		printf("Usage ./%s filename [subproblem]\n","main");
		return 1;
	}
	*/
	const char * filename = argv[1];
	filename = "DAC05";	// temporarily hard code to DAC05
       	if( (f = fopen(filename,"r")) == NULL ){
		printf("open file error\n");
		exit(1);
	}

	parse(f,chip); // now `chip' has store subproblems
	fclose(f);
	return chip;
}

// try to avoid zig-zag heuristic
Point traceback_line(int which, int t, const Point & current, DIRECTION dir){
	// heuristic: see if along the direction satisfies...
	Point rtn(current.x+dx[dir],current.y+dy[dir]);
	if( inGrid( rtn ) && grid[which][rtn.x][rtn.y] == t-1 )
		return rtn;
	// failed, search other direction
	vector<Point> nbr = getNbr(current); 
	vector<Point>::iterator iter;
	for(iter = nbr.begin();iter!=nbr.end();iter++){
		rtn = (*iter);
		if( grid[which][rtn.x][rtn.y] == t-1 )
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

int main(int argc, char * argv[]){
	init(argc,argv,&chip);

	// solve subproblem 1
	int idx = 1;
	scanf("%d",&idx);
	Subproblem * pProb = &chip.prob[idx];
	printf("Start to solve subproblem %d\n",idx);
	
	// sort : decide net order
	netcount = pProb->nNet;
	sortNet(pProb,netorder);
	printf("net order:\n");
	for(int i=0;i<pProb->nNet;i++){printf("%d ",netorder[i]);}
	printf("\n");

	// generate blockage bitmap
	initBlock(pProb);

	// route each net
	int i,j;
	N=chip.N;
       	M=chip.M;
	memset(grid,0,sizeof(grid));
	// start to route each net according to decided order
	for(i=0;i<pProb->nNet;i++){
		int which = netorder[i];
		Net * pNet = &pProb->net[which]; // according to netorder
		printf("** Routing net[%d] **\n",which);

		// do Lee's propagation,handles 2-pin net only currently
		int numPin = pNet->numPin;
		Point S = pNet->pin[0].pt; // source
		Point T = pNet->pin[1].pt; // sink
		for(int i=0;i<numPin;i++)
			cout<<"\tpin["<<i<<"]:"<<pNet->pin[i].pt<<endl;

		// initialize the queue
		deque<GridPoint> p,q;
		p.push_back(GridPoint(0,S));
		deque<GridPoint>::iterator qit;
		int t=0;
		bool success = false;
		do{// propagate process
			t++;
			if( t > MAXTIME ){ // timing constraint
				printf("Exceed route time!\n");
				exit(1);
			}
#ifdef DEBUG
			printf("t=%d\n",t);
#endif
			q=p;
			p.clear();
			// handle wave_front
			for(qit = q.begin(); qit != q.end(); ++qit ){
#ifdef DEBUG
				cout<<"Propagating "<<(*qit).pt<<endl;
#endif
				// check if it is the sink
				if( (*qit).pt == T ) {
					cout<<"Find "<<T<<"!"<<endl;
					success = true;
					break;
				}
				// get its neighbours
				vector<Point> nbr = getNbr((*qit).pt);
				vector<Point>::iterator iter;
				// enqueue neighbours
				for(iter = nbr.begin();iter!=nbr.end();iter++){
					int x=(*iter).x,y=(*iter).y;
					if( *iter != S && // S should not be propagated again
						grid[which][x][y] == 0){ //not routed yet

						// calculate its weight
						Point tmp(x,y);
						if( blockage[x][y] ) grid[which][x][y] = INF;
						else if( fluidicCheck( tmp ) == false ) grid[which][x][y] = INF;
						else if( electrodeCheck( tmp ) == false ) grid[which][x][y] = INF;
						else {
							grid[which][x][y] = t;
#ifdef DEBUG
							cout<<"\tPoint "<<tmp<<" pushed."<<endl;
#endif
							p.push_back( GridPoint(t,tmp) );
						}
					}
				}
			}
		}while(!success); // end of propagate

		if( success == false ){// failed to find path
			fprintf(stderr,"Error: failed to find path\n");
			exit(1);
		}
		else
			printf("Success - start to backtrack\n");
		
		// backtrack phase
		int arrive_time = grid[which][T.x][T.y];
		Point back,new_back=T;
		cout<<"Route:\n"<<setw(8)<<arrive_time<<" : "<<new_back<<endl;
		DIRECTION dir = STAY;
		for(j=arrive_time;j<=chip.time;j++) path[which][j] = T;
		for(j=arrive_time;j>=1;j--){
			// find the next backtrack grid
			dir = PtRelativePos(new_back,back);
			back=new_back;
			new_back = traceback_line(which,j,back,dir);
			cout<<setw(8)<<j-1<<" : "<<new_back<<endl;
			// if impossible, report error
			path[which][j-1] = new_back;
		}
		printf("==========================\n\n");
	}
	return 0;
}
