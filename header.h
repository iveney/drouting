#ifndef __HEADER_H__
#define __HEADER_H__

#include <iostream>
#include <vector>
#include "heap.h"
using std::ostream;
using std::vector;

#define ABS(a) ((a)<0.0?(-(a)):(a))
#define MHT(s,t) (ABS((s.x)-(t.x)) + ABS((s.y)-(t.y)))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)>(b)?(b):(a))

typedef unsigned char BYTE ;
typedef BYTE Grid ;

const int MAXBUF=256;
const int MAXSTR=30;
const int MAXBLK=10;
const int MAXNET=10;
const int MAXSUB=100;
const int MAXGRID=30;
const int MAXTIME=20;
const Grid INF=2<<7-1;
const int FLUID_PENALTY=20;
const int ELECT_PENALTY=20;
const int STALL_PENALTY=20;

class Point{// a point denote by (row,col)
public:
	Point(){x=y=0;}
	Point(int xx,int yy):x(xx),y(yy){}
	bool operator == (const Point & pt) const {return x==pt.x && y==pt.y;}
	bool operator != (const Point & pt) const {return !operator==(pt);}
	friend ostream &operator <<(ostream&,Point&);
	int x,y;
};

enum CORNER{LL,UR};
class Block{// a block denote by two points
public:
	char name[MAXSTR];
	Point pt[2]; // LL=0, UR=1
};

class Pin{// a pin has a name and a location
public:
	char name[MAXSTR];
	Point pt;
};

class Net{// a net has a name and at most 3 pins
public:
	char name[MAXSTR];
	int numPin;
	Pin pin[3];
};

class Subproblem{// a subproblem has some blocks and nets
public:
	int nBlock;
	Block block[MAXBLK];
	int nNet;
	Net net[MAXNET];
};

class Chip{// a chip has an array, timing constraint and subproblems
public:
	int N,M,T;	// array size,timing constraint
	int time;
	int nSubProblem;
	Subproblem prob[MAXSUB];// note:prob starts from index 1
};

class GridPoint{
public:
	GridPoint(Point pt_=Point(0,0),GridPoint *par=NULL,
		int t=0,int b=0,int f=0,int e=0,int s=0,int d=0):
		pt(pt_),parent(par),time(t),bend(b),
		fluidic(f),electro(e),stalling(s),distance(d){
			updateWeight();
		}

	// TODO: how to determine there size if weights are equal?
	// because we need a strictly weak ordering here for heap comparison...
	bool operator < (const GridPoint& g) const{ 
		/*
		if( this->time != g.time )
			return g.time - this->time;
		else
		*/
		return weight < g.weight; 
	}

	/////////////////////////////////////////////////////////////////
	Point pt;		// its position
	GridPoint * parent;     // from which GridPoint it was propagated
	int weight;

	// weight is the sum of:
	int time;
	int bend;
	int fluidic;
	int electro;
	int stalling;
	int distance;  // the manhattance distance to the sink

	int updateWeight(){
		int old = weight;
		weight = time+bend+fluidic+electro+stalling+distance;
		return old;
	}

	class GPpointerCmp {
	public:
		bool operator()(const GridPoint *a, const GridPoint *b) const 
		{ return *a < *b; }
	};
};

class NetRouter{
	public:
		~NetRouter(){
			for(size_t i=0;i<resource.size();i++) delete resource[i];
		}
		int size() const{ return resource.size(); }
		bool route(){
			return true;
		}

		vector<GridPoint*> resource;      // a pointer collection 
		// gp_heap;
};

#endif
