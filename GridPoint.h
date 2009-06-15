#ifndef __GRIDPOINT_H__
#define __GRIDPOINT_H__

#include "stdlib.h"
#include "header.h"

class GridPoint{
public:
	GridPoint(Point pt_=Point(0,0),GridPoint *par=NULL,
	int t=0,int l=0,double c=0.0,int b=0,int s=0,int d=0);

	// copy constructer
	//GridPoint & operator = (const Gridpoint & gp);

	double updateWeight();

	// TODO: how to determine there size if weights are equal?
	// because we need a strictly weak ordering here for heap comparison..
	//bool operator < (const GridPoint& g) const;
	bool operator == (const GridPoint &g) const;
	friend ostream & operator <<(ostream & out,const GridPoint & g);

	/////////////////////////////////////////////////////////////////
	Point pt;		// its position
	GridPoint * parent;     // from which GridPoint it was propagated
	double weight;

	// weight is the sum of:
	int time;
	int length;
	double cell;    // will be multiplied by a factor
	int bend;       // NOW no use in Router.cpp
	int stalling;   // NOW no use in Router.cpp
	int distance;   // the manhattance distance to the sink
	int order;      // order the GridPoint was generated
	static int counter;
	class GPpointerCmp{
	public:
		// true if `a' if less important
		bool operator()(const GridPoint *a, const GridPoint *b) const{
			// precedence:
			// 1. weight
			// 2. MHT
			// 3. time
			//if( a->weight == b->weight ){
			if( EQU(a->weight,b->weight)) {
				if( a->distance == b->distance ){
					if( a->time == b->time)
						return (a->order) > (b->order);
					else
						return a->time < b->time;
				}
				else
					return (a->distance) > (b->distance);
			}
			else
				return GT(a->weight,b->weight);
		}
	};
};

#endif
