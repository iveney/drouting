#ifndef __ROUTER_H__
#define __ROUTER_H__

#include <deque>
#include <vector>
#include <cassert>
#include <string>
#include "header.h"
#include "GridPoint.h"
#include "heap.h"
#include "util.h"
#include "ConstraintGraph.h"
using std::deque;
using std::vector;
using std::string;

// return value of fluidic constraint check
enum FLUIDIC_RESULT{SAFE,VIOLATE,SRC_VIOLATE,SAMENET,SAMEDEST};

// marks the functional part on chip
enum FUNCTION_PLACE{FREE,BLOCK,WASTE};

// implements simple set function
// keeps the ORDER of element insertion
// count the conflict net
class ConflictSet{
public:
	ConflictSet(int num):net_num(num),max_id(-1),total(0),
		conflict_count(IntVector(net_num,0)){
	}
	
	// increment a net's count by 1
	void increment(int net_id){
		int tmp = ++conflict_count[net_id];
		total++;
		if( max_id == -1 || conflict_count[max_id] < tmp ){
			// update max conflict net
			max_id = net_id;
		}
	}

	// merge 2nd ConflictSet to this
	void add_nets(const ConflictSet & toadd){
		for (int i = 0; i < net_num; i++) {
			conflict_count[i]+=toadd.conflict_count[i];
			total+=toadd.conflict_count[i];
			// update max conflict net
			if( max_id == -1 || 
			    conflict_count[i] > conflict_count[max_id] ){
				max_id = i;
			}
		}
	}

	int net_num;		// total number of nets
	int max_id;		// which net causes most conflict
	int total;		// total number of conflicts
	IntVector conflict_count; // each net's conflict count
};

// struct tracks the routing information of a net
struct NetRoute{
	NetRoute(int netidx,int pin_num,int timing_):idx(netidx),
	num_pin(pin_num),timing(timing_) {
	}
	void clear(){
		pin_route[0].clear();
		if( num_pin == 3 ) pin_route[1].clear();
	}
	int idx;	// this net's id
	int num_pin;    // net pins,be 2 or 3
	int timing;     // total time used
	int merge_time; // merge time for this net
	// The route location of this net,
	// at most 3pin, hence only two elements
	PtVector pin_route[2]; 
	int reach_time[2]; // the time a droplet finishes routing
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
	RouteResult(int T_,int W_,int H_,Subproblem * subprob):
		T(T_),W(W_),H(H_),pProb(subprob),
		v_row(vector< vector<COLOR> >(T+1)),
		v_col(vector< vector<COLOR> >(T+1)), // T for dummy use
		activated(vector< PtVector >(T+1))
	{
		// allocate route solution space for each net
		for (int i = 0; i < subprob->nNet; i++) {
			path.push_back(NetRoute(i,get_pinnum(i),T));
		}
		for (int i = 0; i <=T; i++) {
			v_row[i] = vector<COLOR>(H);
			v_col[i] = vector<COLOR>(W);
			activated[i] = PtVector();
		}
	}

	// data members
	// timing constraint, should be the same as the chip->T
	int T,W,H;

	// the corresponding subproblem
	// This includes all the information we need
	Subproblem *pProb;  

	// routing path for each net,size = prob->nNet
	// i.e. given net index and time, we get the position
	// 1st dimension = net, 2nd dimension = time
	vector< NetRoute > path;
	
	// voltage assignment for each time step from 1 up to T
	// at each t, there is a list of H and a list of L, those not
	// in these two lists are assume to be G(ground)
	// 1st dimension = time, 2nd dimenstion = row/col index
	vector< vector<COLOR> > v_row;
	vector< vector<COLOR> > v_col;

	// a list of activated cell at time t
	vector< PtVector > activated;
};

// main class for routing the droplets net
class Router{
public:
	typedef heap<GridPoint*,vector<GridPoint*>,
		     GridPoint::GPpointerCmp > GP_HEAP;

	// default constructor: mark the router's input be empty
	// initialize the netorder vector(default order:1,2,...)
	Router();

	// free the resources allocated
	~Router();

	// read the file and subproblem number from cmd line argument
	void read_file(int argc, char * argv[]);

	void output_result(RouteResult & result);

	// output all the members in current heap
	void output_heap(const GP_HEAP & h);

	// output voltage assignment status;
	void output_voltage(RouteResult & result);
	
	// solve all the subproblems
	ResultVector solve_all();
	
	// solve a subproblem with index=prob_idx
	// returns the routing result
	RouteResult solve_subproblem(int prob_idx);

	// solve the problem given in cmd line
	// returns the routing result of all problems
	ResultVector solve_cmdline();

	// determine if given point pt is in valid position
	bool in_grid(const Point & pt);

	// gets the neighbour points of a point
	PtVector get_neighbour(const Point & pt);

	int get_maxt() const;

private:
	// ******************************************************************//
	// methods
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
	void init_place(Subproblem *p);

	bool route_2pin(int which,RouteResult & result,
		ConflictSet & conflict_net);
	
	bool route_3pin(int which,RouteResult & result,
		ConflictSet & conflict_net);

	void init();

	// do rip up and re route for a net
	bool ripup_reroute(int which,RouteResult & result,
			ConflictSet &conflict_net);


	// given a net index, route the net
	bool route_net(int which,RouteResult &result);

	// determines if there is fluidic constraint violation
	FLUIDIC_RESULT fluidic_check(int which, int pin_idx,
			const Point & pt,int t,
			const RouteResult & result,
			ConflictSet & conflict_set);

	// determines if there is electrode constraint violation
	// 2nd Point is parent's location
	bool electrode_check(int which, int pin_idx,
			const Point & pt, const Point & parent_pt,int t,
			const RouteResult & result,
			ConflictSet & conflict_net,int control);

	bool check_droplet_conflict(
		const Point & S1, const Point & T1,
		const Point & S2, const Point & T2,
		ConstraintGraph * p_graph,
		int t,int control);

	bool try_add_edge(NType ntype,int lineid,
		const Point & S, const Point & T,
		int t,ConstraintGraph * p_graph);

	PtVector geometry_check(NType ntype,int line,
		const Point & S,const Point & T);

	PtVector geometry_check_V(int vline,const Point & S,const Point & T);
	PtVector geometry_check_H(int hline,const Point & S,const Point & T);


	void update_graph(int which,int pin_idx,
			const PtVector & pin_path,
		const RouteResult & result);

	void allocate_graph();
	void destroy_graph();
	
	// backtrack phase, store the result
	void backtrack(int which,int pin_idx,
			GridPoint *current,RouteResult &result);

	// given a point `current',propagate it.
	bool propagate_nbrs(int which, int pin_idx,GridPoint * current,
			Point & dst,RouteResult & result,
			GP_HEAP & p,ConflictSet & conflict_net);

	bool route_subnet(Point src,Point dst,
			int which,int pin_idx,
			RouteResult & result,
			ConflictSet & conflict_net);

	// ******************************************************************//
	// members
	//
	bool read;      // mark if configuration has been read
	Chip chip;      // stores all the information
	int tosolve;    // which subproblem to solve,given in cmd line
	ResultVector route_result; // stores the final route result

	string filename;	// TEST: filename to output the tex file
	int max_t;		// TEST: output maximum time used

	int last_ripper_id;     // marks which net is the ripper

	deque<int> nets;  			// a list of unrouted nets
	// marks if ((x,y),t) has been visited
	char visited[MAXGRID][MAXGRID][MAXTIME+1];

	ConstraintGraph * graph[MAXTIME+1]; 	// each time step's graph
	BYTE blockage[MAXGRID][MAXGRID];	// bitmap for block
	int netcount;              		// total net number
	int W,H,T;				// W=width, H=height
	int netorder[MAXNET];			// the net routing order
	static Subproblem * pProb; 		// pointer to the current
};

#endif
