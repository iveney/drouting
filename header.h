#ifndef __HEADER_H__
#define __HEADER_H__

#include <iostream>
using namespace std;

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

class Block{// a block denote by two points
public:
	char name[MAXSTR];
	Point pt[2];
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
	GridPoint(){weight=0;pt=Point(0,0);}
	GridPoint(int w,Point p):weight(w),pt(p){}
	int weight;
	Point pt;
	// note that the small element wins
	bool operator < (const GridPoint& g){ return weight >= g.weight; }
};

#endif
