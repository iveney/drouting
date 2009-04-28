#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <algorithm>
#include <assert.h>
#include "header.h"
#include "GridPoint.h"
#include "util.h"
#include "Router.h"
#include "parser.h"
#include "heap.h"
using std::cout;
using std::cerr;
using std::endl;
using std::sort;
using std::setw;

int dx[]={-1,1,0,0,0};
int dy[]={0,0,1,-1,0};
extern const char *color_string[];

Subproblem * Router::pProb=NULL;

Router::Router():read(false){
	for(int i=0;i<MAXNET;i++) netorder[i]=i;
}

Router::~Router(){
}


void Router::read_file(int argc, char * argv[]){
	FILE * f;
	if(argc<2)
		report_exit("Usage ./main filename [subproblem]\n");

	const char * filename = argv[1];
	if( argc == 3 ) tosolve = atoi(argv[2]);
	else            tosolve = -1;   // not given in cmdline, solve all
	if( (f = fopen(filename,"r")) == NULL )
		report_exit("open file error\n");
	parse(f,&chip); // now `chip' stores subproblems
	fclose(f);
	init();
}

// do initialization for global variables W H T and graph
void Router::init(){
	read=true;
	W=chip.W;
	H=chip.H;
	T=chip.T;
	// generate a series of time frame to store the voltage assignment
	for (int i = 1; i <= T ; i++) {
		ConstraintGraph * p = new ConstraintGraph(W,H);
		graph[i] = p;
	}
}

// solve subproblem `idx'
RouteResult Router::solve_subproblem(int prob_idx){
	if( read == false ) report_exit("Must read input first!");
	cout<<"--- Solving subproblem ["<<prob_idx<<"] ---"<<endl;
	
	pProb = &chip.prob[prob_idx];
	netcount = pProb->nNet;
	sort_net(pProb,netorder);// sort : decide net order
	output_netorder(netorder,netcount);

	// generate blockage bitmap
	init_place(pProb);

	// the result to return
	RouteResult result(this->T,this->W,this->H,this->pProb);

	// start to route each net according to sorted order
	nets.clear();
	nets.insert(nets.begin(),netorder,netorder+netcount);
	int routed_count=0;
	while( !nets.empty() ){
		int which = nets.front();
		nets.pop_front();
		bool success = route_net(which,result);
		if( success == false ){
			// first net can not be routed means fail
			report_exit("Error: route failed!");
		}
		routed_count++;
	}

	// finally, output result
	cout<<"Subproblem "<<prob_idx<<" solved!"<<endl;
	output_result(result);

	// release graph
	for (int i = 1; i <= T; i++) {
		//cout<<"deleting .."<<i<<endl;
		delete graph[i];
	}

	return result ;
}

void Router::output_result(const RouteResult & result){
	int i,j;
	// for each net
	for (i = 0; i < netcount; i++) {
		const NetRoute & net_path = result.path[i];
		// for each subnet in a net
		for (j = 0; j < net_path.num_pin-1; j++) {
			cout<<"net["<<i<<"]:"<<this->T<<endl;
			const PtVector & route = net_path.pin_route[j];
			// output each time step
			for(size_t k=0;k<route.size();k++){
				cout<<"\t"<<k<<":"
				<<route[k]<<endl;
			}
		}
	}
	output_voltage();
}

void Router::output_voltage(){
	int i,j;
	// output the voltage assignment
	for (i = 1; i <= T; i++) {
		cout<<"[t = "<<i<<"]"<<endl;
		ConstraintGraph * p_graph = graph[i];
		cout<<"ROW:\t";
		PtVector activated;
		activated.clear();
		for (j = 0;j<H; j++) {
			COLOR clr = p_graph->node_list[j].color;
			if(clr==G) continue;
			cout<<"("<<j<<"="<<color_string[clr]<<") ";
			for(int k=0;k<W;k++){
				COLOR c_clr = p_graph->node_list[k+H].color;
				if(clr==HI && c_clr==LO||
				   clr==LO && c_clr==HI) {
					activated.push_back(Point(k,j));
				}
			}
		}

		cout<<endl<<"COL:\t";
		for (j = 0; j < W; j++) {
			COLOR clr = p_graph->node_list[j+H].color;
			if(clr==G) continue;
			cout<<"("<<j<<"="<<color_string[clr]<<") ";
		}
		cout<<endl;
		cout<<"Act:\t";
		for(size_t k=0;k<activated.size();k++){
			cout<<activated[k]<<" ";
		}
		cout<<endl;
	}

}

// mark the block location as 1
void Router::init_place(Subproblem *p){
	assert( p != NULL );
	memset(blockage,FREE,sizeof(blockage));
	int i,x,y;
	for(i=0;i<p->nBlock;i++) {
		Block b = p->block[i];
		for(x=b.pt[0].x;x<=b.pt[1].x;x++)
			for(y=b.pt[0].y;y<=b.pt[1].y;y++)
				blockage[x][y]=BLOCK;
	}
}

ResultVector Router::solve_all(){
	for(int i=0;i<chip.nSubProblem;i++)
		solve_subproblem(i);
	return route_result;
}

bool Router::route_subnet(Point src,Point dst,
			int which,int pin_idx,
			RouteResult & result,
			ConflictSet & conflict_net){
	assert( in_grid(src) && in_grid(dst) );

	// do Lee's propagation,handles 2-pin net only currently
	GridPoint *current;

	// initialize the heap
	GP_HEAP p;

	// start time = 0, source point = src, no parent
	GridPoint *gp_src = new GridPoint(src,NULL); 
	gp_src->distance = MHT(src,dst);
	p.push(gp_src); // put the source point into heap

	int t=0;               // current time step
	bool success = false;  // mark if this net is routed successfully
	FLUIDIC_RESULT fluid_result;
	while( !p.empty() ){
		if( p.size() > MAXHEAPSIZE )
			report_exit("Heap exceed!");
		// get wave_front and propagate its neighbour
		//p.sort();
#ifdef DEBUG
		cout<<"------------------------------------------------"<<endl;
		cout<<"[before pop]"<<endl;
		output_heap(p);
#endif 
		current = p.top();
		p.pop();
#ifdef DEBUG
		cout<<"[after pop]"<<endl;
		output_heap(p);
		cout<<"Pop "         <<current->pt
		    <<" at time "    <<current->time
		    <<", queue size="<<p.size()<<endl;
#endif 

		// sink reached, but need to check whether it stays
		// there will block others
		if( current->pt == dst ) {
			int reach_t = current->time;
			bool fail = false;
			// see whether it can stay in destination
			for (int i = reach_t+1; i <= T; i++) {
				fluid_result = fluidic_check(which,
						pin_idx, current->pt,
						i,result,conflict_net);
				if( fluid_result == VIOLATE ){
					fail = true;
					break;
				}
			}
			if( !fail ){// safely enter destination and stay
				cout<<"Find "<<dst
				    <<" at time "<<current->time<<"!"<<endl;
				success = true;
				break;
			}// otherwise, continue to search
		}
		
		// we do not reach sink...propagate this gridpoint
		// do pruning here:
		// 1. MHT>time left(impossbile to reach dest)
		// 2. time exceed
		t = current->time+1;
		int time_left = this->T - current->time;
		int remain_dist = MHT(current->pt,dst);
		if( (remain_dist > time_left) || (t > this->T) ){
			if( p.size() != 0 ) continue;
			else{// fail,try rip-up and re-route
				cerr<<"Exceed route time!"<<endl;
				success = false;
				break;
			}
		}

		// stall at the same position, stall for 1 time step
		GridPoint *same = new GridPoint( current->pt,
				current, t,current->bend,  
				current->fluidic, current->electro, 
				current->stalling+STALL_PENALTY,
				current->distance );
		fluid_result=fluidic_check(which,pin_idx,
			same->pt,same->time,result,conflict_net);

		// handle fluidic check result
		switch( fluid_result ){
		case SAMENET:// multipin net merge
		case SAMEDEST:
			success = true;
			// need to know which net it met and utilize
			// the existing route, now just route as normal
			p.push(same);
			break;
		case VIOLATE:// ignore this GridPoint
			break;
		case SAFE:
			p.push(same);
#ifdef DEBUG
			cout<<"\tAdd:"<<*same<<endl;
#endif
			break;
		}

		// propagate current point
		propagate_nbrs(which,pin_idx,
				current,dst,result,p,conflict_net);
	}// end of propagate
	
	// failed to find path
	if( success == false ){
		p.free();
		return false;
	}
#ifdef DEBUG
	else{ cout<<"Success - start to backtrack"<<endl; }
#endif
	//////////////////////////////////////////////////////////////////
	// This subnet is successully routed
	// backtrack phase, stores results to RouteResult
	backtrack(which,pin_idx,current,result);
	p.free();
	return true;

}

// given a net with index `which' , route all its subnets
bool Router::route_net(int which,RouteResult &result)//, ConflictSet &conflict_net)
{
	cout<<"** Routing net["<<which<<"] **"<<endl;
	Net * pNet = &pProb->net[which];
	output_netinfo(pNet);
	Point src;
	Point dst = get_netdst_pt(which);
	// decompose to (numPin-1) subnet
	for (int i = 0; i < pNet->numPin-1; i++) {
		src=pNet->pin[i].pt; // route pin-i
		ConflictSet conflict_net(netcount);
		bool success = route_subnet(src,dst,which,i,
			result,conflict_net);
		if( success == false ){
			if(netorder[0]==which) // first net can not be route!
				return false;
			cerr<<"Error: route subnet "<<which<<"-"<<i
			    <<" failed ,try ripup-reroute!"<<endl;
			ripup_reroute(which,result,conflict_net);//-1;
			output_netorder(netorder,netcount);
			i--; // re-route this one
		}
		//output_result(result);
	}
	return true;
}

// for a given net `which', find a valid net order to reroute it
// POSTCONDITION: the net order will be changed
// RETURN: the 
int Router::ripup_reroute(int which,RouteResult & result,
		ConflictSet &conflict_net){
	// cancel the route result of some conflict net
	// now use the last conflict net
	int last = conflict_net.get_last();
	int last_idx,which_idx;
	for(int i = 0; i < netcount; i++) {
		if( netorder[i] == last ) last_idx = i;
		if( netorder[i] == which) which_idx = i;
	}
	result.path[last].clear(); // cancel the routed path

	// re-push into queue: re route `last'
	nets.push_front(last);

	// re-order route order
	IntVector temp(netorder,netorder+netcount);
	temp.erase(temp.begin()+last_idx);  // remove net=last
	temp.insert(temp.begin()+which_idx,last);// insert it after which
	copy(temp.begin(),temp.end(),netorder);
	return which_idx-1;// net=which moved one place before in netorder
}

// given a point `current' during propagation stage of routing net `which',
// generate its neighbours push into heap if satisfies constraint.
// dst : the sink point, 
// result: store the routing result
// p: heap
// conflict_net : keep info. of the conflicting net
void Router::propagate_nbrs(int which, int pin_idx,GridPoint * current,
			    Point & dst,RouteResult & result,
			    GP_HEAP & p,ConflictSet &conflict_net){
	// get its neighbours( PROBLEM: can it be back? )
	PtVector nbr = get_neighbour(current->pt);
	const int t = (current->time + 1);

	// enqueue neighbours
	GridPoint * par_par = current->parent; 
	for(size_t i=0;i<nbr.size();i++){
		int x=nbr[i].x,y=nbr[i].y;
		// 0.current pt should be avoided
		// 1.parent should not be propagated again
		// 2.check if there is blockage 
		// 3.forbid circular move (1->2...->1)
		if( nbr[i] == current->pt ) continue;
		if( blockage[x][y] == BLOCK ) continue;
		if( (par_par != NULL) && 
		    (nbr[i] == par_par->pt) )
			continue;
		// calculate its weight
		Point tmp(x,y);
		int f_pen=0,e_pen=0,bending=current->bend;
		FLUIDIC_RESULT fluid_result=fluidic_check(which,pin_idx,
				tmp,t,result,conflict_net);
		bool elect_violate=electrode_check(which,pin_idx,
			       	tmp,current->pt,t,result,conflict_net,0);

		// fluidic constraint check
		if( fluid_result == SAMENET ){
			// multipin net
		} else if( fluid_result == VIOLATE ){
			continue;
		}

		// electro constraint check
		if( !elect_violate ){
			continue;
		}

		// bending update
		if( par_par != NULL &&
		    check_bending(tmp,par_par->pt) == true )
			bending++;

		GridPoint *nbpt = new GridPoint(
				tmp,current,
				t, bending,
				f_pen, e_pen,
				current->stalling,
				MHT(tmp,dst));
		p.push(nbpt);
#ifdef DEBUG
		cout<<"\tAdd t="<<t<<":"<<*nbpt<<endl;
#endif
	}// end of enqueue neighbours
}

// do backtrack for net `which', 
// from desination point `current',
// store the path into `result'
void Router::backtrack(int which,int pin_idx,GridPoint *current,
		RouteResult & result){
	// output format:(e.g.)
	// net[0]:20
	//     0: (1,3)
	//     1: (2,3)
	//     2: ...
	//     20:(21,3)
	//
	PtVector & pin_path = result.path[which].pin_route[pin_idx];
	Point dst = current->pt;
	while( current != NULL ){// trace back from dest to src
		pin_path.insert(pin_path.begin(),current->pt);
		// we need to update all the coloring status 
		// for every time step until it reaches the dest, 
		// however, do not add the stalling frame!
		if( current->parent == NULL ) break;
		DIRECTION dir = pt_relative_pos(current->parent->pt,
				current->pt);
		if( dir != STAY ){
			int t = current->time;
			ConstraintGraph * p_graph = graph[t];
			GNode add_x(COL,current->pt.x),
			      add_y(ROW,current->pt.y);
			p_graph->add_edge_color(add_x,add_y,DIFF);
		}
		current = current->parent;
	}

	// IMPORATANT: for waste disposal point
	// DO NOT need to generate STAY result
	// e.g. reaches at t=13, then disppear from t=14
	if( dst == chip.WAT ) return;

	// NOTE: that some droplet may reach the destination earlier than the
	// timing constraint. fill all later time with its final position
	for(int i=pin_path.size();i<=this->T;i++){
		pin_path.push_back(dst); // dst is destination pt here
	}

	// output result
	//output_result(result);
}

// just print out what is in the heap
void Router::output_heap(const GP_HEAP & h){
	for(int i=0;i<h.size();i++) {
		cout<<i<<":"<<*(h.c[i])<<endl;
	}
}

// test if a point is in the chip array
bool Router::in_grid(const Point & pt){
	if( pt.x >=0 && pt.x <W && 
	    pt.y >=0 && pt.y <H) return true;
	else return false;
}

// get the neighbour points of a point in the chip
PtVector Router::get_neighbour(const Point & pt){
	PtVector s;
	for(int i=0;i<4;i++){
		Point p(pt.x+dx[i],pt.y+dy[i]);
		if( in_grid(p) )
			s.push_back(p);
	}
	return s;
}

// Compare which net should be routed first
// note: this is a static function 
// otherwise the signature makes qsort unusable
// also the member `pProb' is set to static in order to be accessed
// RESULT: smaller value goes first
int Router::cmp_net(const void* id1,const void* id2){
	int i1 = *(int*)id1;
	int i2 = *(int*)id2;
	Net * n1 = &pProb->net[i1];
	Net * n2 = &pProb->net[i2];
	Block b1,b2;
	// get two bounding box
	b1 = get_bbox(n1->pin[0],n1->pin[1]);
	b2 = get_bbox(n2->pin[0],n2->pin[1]);
	// pin1's source inside bounding box 2,should route 1 first
	if( pt_in_rect(b2,n1->pin[0].pt) ) return -1;
	// pin2's source inside bounding box 1,should route 2 first
	else if( pt_in_rect(b1,n2->pin[0].pt) ) return 1;

	// use manhattance to judge
	int m1 = MHT(n1->pin[0].pt,n1->pin[1].pt);
	int m2 = MHT(n2->pin[0].pt,n2->pin[1].pt);
	return m1-m2;
}

void Router::sort_net(Subproblem *pProb, int * netorder){
	int num=pProb->nNet;
	qsort(netorder,num,sizeof(int),cmp_net);
}

void Router::output_netinfo(Net *pNet){
	for(int i=0;i<pNet->numPin;i++)// output Net info
		cout<<"\t"<<"pin "<<i<<":"<<pNet->pin[i].pt<<endl;
}

void Router::output_netorder(int *netorder,int netcount){
	cout<<"net order: [ ";
	for(int i=0;i<netcount;i++) cout<<netorder[i]<<" ";
	cout<<"]"<<endl;
}

// control=0: test =1:real
// a droplet `d1' is moving to a point `pt' at time `t', 
// d1 is `pin_idx' subnet of net `which'
//
// determine whether it will violate electrode constraint
// 1  2  3  4
// 5  6->7  8
// 9  10 11 12
// suppose move 6->7, then row 5, col 3 (C3,R5) activated(type 1 constraint)
// ensure it will not be affected by others if (C3,R5) activated(type 2)
// ensure 2 nets will not cause conflict on another net:1,2,4,9,10,12(type 3)
bool Router::electrode_check(int which, int pin_idx,
		const Point & pt,const Point & parent_pt,int t,
		const RouteResult & result,
		ConflictSet & conflict_net,int control){
	// no need to check at time t=0, no row/col activate
	if( t == 0 ) return true; 
	
	// add edge into the graph of this time step
	ConstraintGraph * p_graph = graph[t];
	ConstraintGraph testgraph(*p_graph);
	if( control == 0 ) p_graph = &testgraph; // use for testing
	GNode add_x(COL,pt.x), add_y(ROW,pt.y);  // y is row
	bool add_result;
	
	// TYPE 1:
	// try coloring in this time step
	// IMPORTANT: if the droplet stays, ignore this
	DIRECTION dir1 = pt_relative_pos(parent_pt,pt);
	if( dir1 != STAY ){
		add_result = p_graph->add_edge_color(add_x,add_y,DIFF);
		if( add_result == false ) return false;
	}

	// let another droplet be `d2'
	for (int i = 0; i<netcount && netorder[i]!=which; i++) {
		int checking_idx = netorder[i];
		const NetRoute & route = result.path[checking_idx];
		// for each subnet(at most 2)
		for (int j = 0; j<route.num_pin-1; j++) {
			const PtVector & pin = route.pin_route[j];
			// pin[t] is the location at time t(activated at t-1)
			// Type 2: check if net-which+other-net affect d2
			if( dir1 != STAY ){
				add_result=check_droplet_conflict(parent_pt,pt,pin[t-1],pin[t],p_graph,t,control);
				if( add_result == false ) return false;
			}
			// Type 3: check if d2+other-net affect net-which
			DIRECTION dir2 = pt_relative_pos(pin[t-1],pin[t]);
			if( dir2 != STAY ){
				add_result=check_droplet_conflict(pin[t-1],pin[t],parent_pt,pt,p_graph,t,control);
				if( add_result == false ) return false;
			}

		} // end of for j
	} // end of for i

	// check 3-pin net here
	if( get_pinnum(which) == 3 && pin_idx == 1 ){
		// if satisfies merging condition ...
		cout<<"2nd pin of 3-pin net "<<which<<endl;
	}

	// no electro constraint violation!
	return true;
}

// check whether droplet d1 will cause conflict on droplet d2 at time t
// where S1,T1 are d1's location
// where S2,T2 are d2's location
bool Router::check_droplet_conflict(
		const Point & S1, const Point & T1,
		const Point & S2, const Point & T2,
		ConstraintGraph * p_graph,
		int t,int control){
	DIRECTION dir1 = pt_relative_pos(S1,T1);
	if( dir1 == STAY ) return true;

	bool result;
	// check if the activation of a row affect d2
	result = try_add_edge(ROW,T1.y,S2,T2,t,p_graph);
	if( result == false ) return false;

	// check if the activation of a col affect d2
	result = try_add_edge(COL,T1.x,S2,T2,t,p_graph);
	if( result == false ) return false;

	return true;
}

// hline is the horizontal line num
// S and T are the droplet's position
PtVector Router::geometry_check_H(int hline,const Point & S,const Point & T){
	assert(hline>=0);
	PtVector pts,cells(3);
	const int xs[]={T.x-1,T.x,T.x+1};
	for (int i = 0; i < 3; i++) 
		if( xs[i] >= 0 ) cells.push_back( Point(xs[i],hline) );
	DIRECTION dir = pt_relative_pos(S,T);

	// if the line not intersect with 5x5 bounding box(BB) of T
	int delta = hline - T.y;
	if( ABS(delta) > 2 ) return pts; // empty
	if( ABS(delta) <=1 ){// intersect with 3x3 BB
		pts = cells;
		switch( dir ){
			case LEFT: 
				if( S.x+1 >= 0 )
					pts.push_back(Point(S.x+1,S.y));
				break;
			case RIGHT:
				if( S.x-1 >= 0 )
					pts.push_back(Point(S.x-1,S.y));
				break;
			default:break; // stalling or moving vertically
		}
	}
	else if( (dir == UP   && hline == S.y-1) || 
		 (dir == DOWN && hline == S.y+1)) {// ABS(delta) = 2
		// intersects with upper or lower boundary of 5x5 BB
		pts = cells;
	}
	return pts;
}

// vline is the vertical line num
// S and T are the droplet's position
// IMPORTANT: remember to check whether a point is in the chip
PtVector Router::geometry_check_V(int vline,const Point & S,const Point & T){
	assert( vline >= 0 );
	PtVector pts,cells(3);
	const int ys[]={T.y-1,T.y,T.y+1};
	for (int i = 0; i < 3; i++) 
		if( ys[i] >= 0 ) cells.push_back( Point(vline,ys[i]) );
	DIRECTION dir = pt_relative_pos(S,T);

	// if the line not intersect with 5x5 bounding box(BB) of T
	int delta = vline - T.x;
	if( ABS(delta) > 2 ) return pts; // empty
	if( ABS(delta) <=1 ){// intersect with 3x3 BB
		pts = cells;
		switch( dir ){
			case UP: 
				if( S.y-1 >= 0 )
					pts.push_back(Point(vline,S.y-1));
				break;
			case DOWN:
				if( S.y+1 >= 0 )
					pts.push_back(Point(vline,S.y+1));
				break;
			default:break; // stalling or moving vertically
		}
	}
	else if( (dir == LEFT  && vline == S.x+1) || 
		 (dir == RIGHT && vline == S.x-1)) {// ABS(delta) = 2
		// intersects with upper or lower boundary of 5x5 BB
		pts = cells;
	}
	return pts;
}

PtVector Router::geometry_check(NType ntype,int line,
		const Point & S,const Point & T){
	if(ntype == ROW) return geometry_check_H(line,S,T);
	else return geometry_check_V(line,S,T);
}


// add an edge to a constraint graph
// ntype:  be ROW or COL
// lineid: the line's index
// t    :  current time
// S    :  the droplet's position at t-1
// T    :  the droplet's position at t
bool Router::try_add_edge(NType ntype,int lineid,
		const Point & S, const Point & T,
		int t,ConstraintGraph * p_graph){
	PtVector pts = geometry_check(ntype,lineid,S,T);
	bool result;
	for (size_t i = 0; i < pts.size(); i++) {
		// get the information of cell i in graph
		if( ntype == ROW ){
			// get the node of col pts[i].x
			GNode col_node(COL,pts[i].x);
			GNode row_node(ROW,lineid);
			if( p_graph->get_node_color(col_node) != G ){
				result = p_graph->add_edge_color(col_node,row_node,SAME);
				if(result == false) return false;
			}
		}	
		else{// COL
			GNode col_node(COL,lineid);
			GNode row_node(ROW,pts[i].y);
			if( p_graph->get_node_color(row_node) != G ){
				result = p_graph->add_edge_color(col_node,row_node,SAME);
				if(result == false) return false;
			}
		}
	}
	return true;
}

// for a net `which' at location `pt' at time `t', 
// perform fluidic constraint check.if successful return 0 
// else return the conflicting net
#define fluidic_violate(pt,t,ep) \
	(!(abs((pt).x - path[(t)-(ep)].x) >=2 || \
	   abs((pt).y - path[(t)-(ep)].y) >=2))
#define static_violate(pt,t) \
	fluidic_violate(pt,t,0)
#define dynamic_violate(pt,t) \
	fluidic_violate(pt,t,1)
FLUIDIC_RESULT Router::fluidic_check(int which,int pin_idx,
	       	const Point & pt,int t,
		const RouteResult & result,ConflictSet & conflict_net){
	// for each routed net(including partial), 
	// check if the current routing net(which) violate fluidic rule
	// we have known the previous routed net 
	// from netorder[0] to netorder[i]!=which
	// t's range: [0..T]
	assert( (t <= this->T) && (t >= 0) );
	// for all the net routed before which
	for(int i=0;i<netcount && netorder[i]!=which;i++){
		int checking_idx = netorder[i];
		// NOTE: if two nets have same dest, they are going to
		// the waste disposal point
		//if( net_same_dest(which,checking_idx) ) return SAMEDEST;
		const NetRoute & route = result.path[checking_idx];
		// for each subnet
		for (int j = 0; j < route.num_pin-1; j++) {
			const PtVector & path = route.pin_route[j];
			// static fluidic check
			//if ( !(abs(pt.x - path[t].x) >=2 || abs(pt.y - path[t].y) >=2) ){
			if( static_violate(pt,t) || 
		            dynamic_violate(pt,t) ){
				conflict_net.insert(checking_idx);
				return VIOLATE;
			}
			// dynamic fluidic check
			// IMPORTANT!!
			// how to treat this case? 1=t,2=t+1 position
			// 1 2 1 2
			// the second 1 will have two direction to go
			// but both 1 belong to the same net!!
			//if ( !(abs(pt.x - path[t-1].x) >=2 || abs(pt.y - path[t-1].y) >=2) ){
		} // end of for j
	} // end of for i

	// check whether it hit into inself
	const NetRoute & route = result.path[which];
	// for all the subnet routed before this pin
	// only need to do it for 3-pin net
	if( route.num_pin == 3 ){
		const PtVector & path = route.pin_route[0];
		if( static_violate(pt,t) || dynamic_violate(pt,t) ){
			return SAMENET;  // they should merge
		}
	}
	return SAFE;
}

ResultVector Router::solve_cmdline(){
	// tosolve is not given in cmdline
	if( this->tosolve == -1 ) 
		route_result = solve_all();
	else
		route_result.push_back(solve_subproblem(tosolve));
	return route_result;
}
