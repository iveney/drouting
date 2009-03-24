#ifndef __GRIDPOINT_H__
#define __GRIDPOINT_H__

#include "stdlib.h"
#include "header.h"

class GridPoint{
public:
	GridPoint(Point pt_=Point(0,0),GridPoint *par=NULL,
	int t=0,int b=0,int f=0,int e=0,int s=0,int d=0);

	int updateWeight();

	// TODO: how to determine there size if weights are equal?
	// because we need a strictly weak ordering here for heap comparison..
	bool operator < (const GridPoint& g) const;
	bool operator == (const GridPoint &g) const;
	friend ostream & operator <<(ostream & out,const GridPoint & g);

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
	int distance;   // the manhattance distance to the sink
	int order;      // orde the GridPoint was generated
	static int counter;
	class GPpointerCmp{
	public:
		bool operator()(const GridPoint *a, const GridPoint *b) const{
			if( a->weight == b->weight )
				return (a->order) > (b->order);
			else
				return a->weight > b->weight;
		}
	};
};

#endif
