#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "header.h"
#include "GridPoint.h"
#include "heap.h"
#include "util.h"

class RouteResult{
public:
};

// main class for routing the droplets net
class Router{
public:
	typedef heap<GridPoint*,vector<GridPoint*>,
		     GridPoint::GPpointerCmp > GP_HEAP;

	// default constructor: mark the router's input be empty
	Router():read(false){
		for(int i=0;i<MAXNET;i++) netorder[i]=i;
	}

	// free the resources allocated
	~Router(){}

	// read the file and subproblem number from cmd line argument
	void read_file(int argc, char * argv[]);

	// output all the members in current heap
	void output_heap(const GP_HEAP & h);

	// given a net index, route the net
	void route_net(int which);

	// solve all the subproblems
	vector<RouteResult> solve_all();

	// solve a subproblem with index=prob_idx
	RouteResult solve_subproblem(int prob_idx);

	// solve the problem given in cmd line
	vector<RouteResult> solve_cmdline();

	// determines if there is fluidic constraint violation
	int fluidic_check(int which, const Point & pt,int t);

	// determines if there is electrode constraint violation
	bool electrode_check(const Point & pt);

	// determine if given point pt is in valid position
	bool in_grid(const Point & pt);

	// gets the neighbour points of a point
	vector<Point> get_neighbour(const Point & pt);

	///////////////////////////////////////////////////////////////////
	// members
	bool read;      // mark if configuration has been read
	Chip chip;      // stores all the information
	int tosolve;    // which subproblem to solve,given in cmd line
	vector<RouteResult> route_result;

	///////////////////////////////////////////////////////////////////
	// members for internal use of routing
	BYTE blockage[MAXGRID][MAXGRID];
	int netcount;
	int N,M,T;
	int netorder[MAXNET];
	static Subproblem * pProb;

private:
	// sort the net according to some criteria defined in cmp_net
	void sort_net(Subproblem *pProb, int * netorder);

	// be used for qsort to decide netorder
	//int cmp_net(const int id1,const int id2);
	static int cmp_net(const void* id1,const void* id2);

	// output the current netorder
	void output_netorder(int *netorder,int netcount);

	// output information of pNet
	void output_netinfo(Net *pNet);

	// init blockage bitmap for use
	void init_block(Subproblem *p);
};

#endif
