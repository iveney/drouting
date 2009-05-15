#ifndef __HEADER_H__
#define __HEADER_H__

#include <iostream>
#include <vector>
#include "util.h"
#include "heap.h"
using std::ostream;
using std::vector;

#define ABS(a) ((a)<0.0?(-(a)):(a))
#define MHT(s,t) (ABS((s.x)-(t.x)) + ABS((s.y)-(t.y)))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)>(b)?(b):(a))

typedef unsigned char BYTE ;
typedef BYTE Grid ;

const int MAXPIN=3;
const int MAXBUF=256;
const int MAXSTR=30;
const int MAXBLK=10;
const int MAXNET=10;
const int MAXSUB=100;
const int MAXGRID=30;
const int MAXTIME=30;
//const int MAXCFLT=10000;
//const int MAX_SINGLE_CFLT=1000;
const Grid INF=2<<7-1;
const int FLUID_PENALTY=20;
const int ELECT_PENALTY=20;
//const int STALL_PENALTY=20;
const int STALL_PENALTY=0;
const int MAXHEAPSIZE=2<<20;
enum CORNER{LL,UR};

class Point{// a point denote by (row,col)
public:
	Point(){x=y=0;}
	Point(int xx,int yy):x(xx),y(yy){}
	bool operator == (const Point & pt) const {return x==pt.x && y==pt.y;}
	bool operator != (const Point & pt) const {return !operator==(pt);}
	friend ostream &operator <<(ostream&,const Point&);
	friend bool operator < (const Point &a, const Point &b);
	int x,y;
};

struct Block{// a block denote by two points
	char name[MAXSTR];
	Point pt[2]; // LL=0, UR=1
};

struct Pin{// a pin has a name and a location
	char name[MAXSTR];
	Point pt;
};

struct Net{// a net has a name and at most 3 pins
	char name[MAXSTR];
	int numPin;
	Pin pin[3];
};

struct Subproblem{// a subproblem has some blocks and nets
	int nBlock;
	Block block[MAXBLK];
	int nNet;
	Net net[MAXNET];
};

struct Chip{// a chip has an array, timing constraint and subproblems
	int W,H,T;	// array size,timing constraint
	int time;
	int nSubProblem;
	Point WAT;
	Subproblem prob[MAXSUB];// note:prob starts from index 1
};

void swap(int &a,int &b);
Block get_bbox(const Pin & p1, const Pin & p2);
bool pt_in_rect(const Block & bb, const Point & pt);

#endif
