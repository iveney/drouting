#ifndef __HEADER_H__
#define __HEADER_H__

#include <iostream>
using namespace std;

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
	GridPoint():bend(0),fluidic(0),time(0),weight(0),electro(0){
		pt=Point(0,0);
		Parent=Point(0,0);
	}
	GridPoint(Point p,Point par;
		int w=0,int t=0,int b=0,int f=0,int e=0):
		weight(w),time(t),bend(b),fluidic(f),electro(e),
		pt(p),parent(par){}
	// note that the small element wins
	bool operator < (const GridPoint& g){ 
		return weight >= g.weight; 
	}

	Point parent;
	Point pt;
	int weight;
	int time;
	int bend;
	int fluidic;
	int electro;
};

#endif
