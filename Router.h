#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "header.h"
#include "GridPoint.h"
#include "heap.h"
#include "util.h"

class RouteResult{
public:
};

class Router{
public:
	typedef heap<GridPoint*,vector<GridPoint*>,GridPoint::GPpointerCmp> GP_HEAP;

	Router():read(false){}
	~Router(){
	//	for(size_t i=0;i<resource.size();i++) delete resource[i];
	}
	void read_file(int argc, char * argv[]);
	void route_net(int which);
	void init_block(Subproblem *p);
	RouteResult solve_subproblem(int prob_idx);
	int fluidic_check(int which, const Point & pt,int t);
	bool electrode_check(const Point & pt);

	//int size() const{ return resource.size(); }
	//vector<GridPoint*> resource;      // a pointer collection 
	// gp_heap;
	
	/////////////////////////////////////////////////
	// members
	bool read;      // mark if configuration has been read
	Chip chip;      // stores all the information
	int tosolve;    // indicates which subproblem to solve

	// members for internal use of routing
	BYTE blockage[MAXGRID][MAXGRID];
	int netcount;
	int N,M;
	int netorder[MAXNET];
	Subproblem * pProb;
	friend int wrapper(const void *id1,const void * id2);
	bool in_grid(const Point & pt);
	vector<Point> get_neighbour(const Point & pt);

private:
	void sort_net(Subproblem *pProb, int * netorder);
	int cmpNet(const void * id1, const void * id2);
	void output_netorder(int *netorder,int netcount);
};

#endif
