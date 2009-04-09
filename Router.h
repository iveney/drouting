#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "header.h"
#include "GridPoint.h"
#include "heap.h"
#include "util.h"

// return value of fluidic constraint check
enum FLUIDIC_RESULT{SAFE,VIOLATE,SAMENET,SAMEDEST};

// implements simple set function
// keeps the ORDER of element insertion
class ConflictSet{
public:
	ConflictSet(int num):net_num(num),
		isConflict(IntVector(net_num,0)){
	}
	void insert(int net_idx){
		if( isConflict[net_idx] ) return;
		isConflict[net_idx] = 1;
		order.push_back(net_idx);
	}
	int get_last(){return order.back();}
	int net_num;
	IntVector isConflict;
	IntVector order;
};

// the class to stores the final routing result
#define get_pinnum(net_id) pProb->net[(net_id)].numPin
#define get_pinpt(net_id,pin_id) pProb->net[(net_id)].pin[(pin_id)].pt
#define get_netdst_pt(net_id) ((get_pinnum(net_id)==2)?\
		get_pinpt((net_id),1):get_pinpt((net_id),2))
#define net_same_dest(id1,id2) (get_netdst_pt(id1)==get_netdst_pt(id2))
class RouteResult{
public:
	// constructor, must use T and prob to initialize
	RouteResult(int T_,int N_,int M_,Subproblem & subprob):
		T(T_),N(N_),M(M_),prob(subprob) {
			path = vector< PtVector >(prob.nNet);
	}

	// data members
	// timing constraint, should be the same as the chip->T
	int T,N,M;

	// the corresponding subproblem
	// This includes all the information we need
	Subproblem prob;  

	// routing path for each net,size = prob->nNet
	// i.e. given net index and time, we get the position
	// 1st dimension = net, 2nd dimension = time
	vector< PtVector > path;
	
	// voltage assignment for each time step from 1 up to T
	// at each t, there is a list of H and a list of L, those not
	// in these two lists are assume to be G(ground)
	// 1st diemstion = time, 2nd dimenstion = some voltage
	vector< IntVector > tHigh;
	vector< IntVector > tLow;
};

// main class for routing the droplets net
class Router{
public:
	typedef heap<GridPoint*,vector<GridPoint*>,
		     GridPoint::GPpointerCmp > GP_HEAP;

	// default constructor: mark the router's input be empty
	// initialize the netorder vector(default order:1,2,...)
	Router():read(false){
		for(int i=0;i<MAXNET;i++) netorder[i]=i;
	}

	// free the resources allocated
	~Router(){}

	// do rip up and re route for a net
	int ripup_reroute(int which,RouteResult & result,
			ConflictSet &conflict_net);

	// read the file and subproblem number from cmd line argument
	void read_file(int argc, char * argv[]);

	// output all the members in current heap
	void output_heap(const GP_HEAP & h);

	// given a net index, route the net
	bool route_net(int which,RouteResult &result,ConflictSet &conflict_net);

	// solve all the subproblems
	ResultVector solve_all();

	// solve a subproblem with index=prob_idx
	// returns the routing result
	RouteResult solve_subproblem(int prob_idx);

	// solve the problem given in cmd line
	// returns the routing result of all problems
	ResultVector solve_cmdline();

	// determines if there is fluidic constraint violation
	FLUIDIC_RESULT fluidic_check(int which, const Point & pt,int t,
		const RouteResult & result,ConflictSet & conflict_net);

	// determines if there is electrode constraint violation
	bool electrode_check(const Point & pt);

	// determine if given point pt is in valid position
	bool in_grid(const Point & pt);

	// gets the neighbour points of a point
	PtVector get_neighbour(const Point & pt);

	// backtrack phase, store the result
	void backtrack(int which,GridPoint *current,RouteResult &result);

	// given a point `current',propagate it.
	void propagate_nbrs(int which,GridPoint * current,
			Point & dst,RouteResult & result,
			GP_HEAP & p,ConflictSet & conflict_net);
	

	///////////////////////////////////////////////////////////////////
	// members
	bool read;      // mark if configuration has been read
	Chip chip;      // stores all the information
	int tosolve;    // which subproblem to solve,given in cmd line
	ResultVector route_result;

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
