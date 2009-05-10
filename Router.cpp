#include <iostream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <assert.h>
#include "header.h"
#include "GridPoint.h"
#include "util.h"
#include "Router.h"
#include "parser.h"
#include "heap.h"
#include "draw_voltage.h"
using std::cout;
using std::cerr;
using std::endl;
using std::sort;
using std::setw;
using std::string;

char visited[MAXGRID][MAXGRID][MAXTIME+1];
int dx[]={-1,1,0,0,0};
int dy[]={0,0,1,-1,0};
extern const char *color_string[];

// parameter to control the searching
int MAXCFLT=10000;
int MAX_SINGLE_CFLT=1000;

// static memeber initialization
Subproblem * Router::pProb=NULL;

// constructer of Router
// initialize the read flag, netorder, and the max_t
Router::Router():read(false),max_t(-1){
	for(int i=0;i<MAXNET;i++) netorder[i]=i;
}

// desctructor
Router::~Router(){
	// release graph
	destroy_graph();
}

// read in a chip file
// read e.g. DAC05 for example
void Router::read_file(int argc, char * argv[]){
	FILE * f;
	if(argc<=2)
		report_exit("Usage ./main filename subproblem\n");
	
	// now only allows to solve a given subproblem
	//if(argc<2) report_exit("Usage ./main filename [subproblem]\n");

	//const char * filename = argv[1];
	filename = string(argv[1]);

	if( argc == 3 ) tosolve = atoi(argv[2]);
	else            tosolve = -1;   // not given in cmdline, solve all
	if( (f = fopen(filename.c_str(),"r")) == NULL )
		report_exit("open file error\n");
	parse(f,&chip); // now `chip' stores subproblems
	fclose(f);
	init();
}

// allocate space for each time step of the graph
void Router::allocate_graph(){
	// index range: 1,2,...T, (totally T graphs)
	for (int i = 1; i <= T ; i++) {
		ConstraintGraph * p = new ConstraintGraph(W,H);
		graph[i] = p;
	}
}

// free the space allocated to constraint graph
void Router::destroy_graph(){
	for (int i = 1; i <= T; i++) {
		//cout<<"deleting .."<<i<<endl;
		delete graph[i];
	}
}

// do initialization for global variables W H T and graph
void Router::init(){
	read=true;
	W=chip.W;
	H=chip.H;
	T=chip.T;
	// generate a series of time frame to store the voltage assignment
	// TEST: set the visited bitmap to empty
	memset(visited,0,sizeof(visited));
	
	allocate_graph();
}

// solve subproblem `prob_idx'
RouteResult Router::solve_subproblem(int prob_idx){
	if( read == false ) report_exit("Must read input first!");
	cout<<"--- Solving subproblem ["<<prob_idx<<"] ---"<<endl;

	pProb = &chip.prob[prob_idx];
	netcount = pProb->nNet;
	sort_net(pProb,netorder);// sort : decide net order
	output_netorder(netorder,netcount);

	last_ripper_id = -1;

	// generate blockage bitmap
	init_place(pProb);

	// the result to return
	RouteResult result(this->T,this->W,this->H,this->pProb);

	// heuristic: set the parameter
	//int div = (pProb->nNet/2);
	//MAX_SINGLE_CFLT = this->W * this->H * this->T /(div==0?1:div) ;
	//MAXCFLT = MAX_SINGLE_CFLT;

	//MAX_SINGLE_CFLT = 100;
	//MAXCFLT = 300;

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

	// TEST: output result to TeX file
	char buf[100];
	sprintf(buf,"%s_%d_sol.tex",filename.c_str(),prob_idx);
	draw_voltage(result,buf);

	return result ;
}

// output the result to standard output
void Router::output_result(RouteResult & result){
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
	output_voltage(result);
}

// output the voltage assignment
// IMPORTANT: also save the voltage assignment result into RouteResult
// from ConstraintGraph
void Router::output_voltage(RouteResult & result){
	int i,j;
	// for each time step
	for (i = 1; i <= T; i++) {
		cout<<"[t = "<<i<<"]"<<endl;
		ConstraintGraph * p_graph = graph[i];
		cout<<"ROW:\t";
		PtVector activated;
		activated.clear();
		result.activated[i-1].clear();
		// output ROW
		for (j = 0;j<H; j++) {
			COLOR clr = p_graph->node_list[j].color;
			result.v_row[i-1][j] = clr;
			if(clr==G) continue;
			cout<<"("<<j<<"="<<color_string[clr]<<") ";
			for(int k=0;k<W;k++){
				COLOR c_clr = p_graph->node_list[k+H].color;
				if(clr==HI && c_clr==LO||
				   clr==LO && c_clr==HI) {
					Point tmp(k,j);
					activated.push_back(tmp);
				}
			}
		}

		// output COL
		cout<<endl<<"COL:\t";
		for (j = 0; j < W; j++) {
			COLOR clr = p_graph->node_list[j+H].color;
			result.v_col[i-1][j] = clr;
			if(clr==G) continue;
			cout<<"("<<j<<"="<<color_string[clr]<<") ";
		}
		cout<<endl;
		cout<<"Act:\t";
		for(size_t k=0;k<activated.size();k++){
			cout<<activated[k]<<" ";
			result.activated[i-1].push_back(activated[k]);
		}
		cout<<endl;
	}
	for(i=0;i<W;i++){
		result.v_col[T][i]=G;
	}
	for(i=0;i<H;i++){
		result.v_row[T][i]=G;
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

// DEPRECATE: solves all the subproblems
ResultVector Router::solve_all(){
	for(int i=0;i<chip.nSubProblem;i++)
		solve_subproblem(i);
	return route_result;
}

// given a net `which', route its `pin_idx' pin
// src = source point
// dst = sink point
// result stores the Route result
// conflict_set stores the conflict count for other nets
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
	GridPoint::counter = 0;
	GridPoint *gp_src = new GridPoint(src,NULL); 
	gp_src->distance = MHT(src,dst);
	p.push(gp_src); // put the source point into heap
	// mark it
	visited[src.x][src.y][0] = 1;

	int t=0;               // current time step
	bool success = false;  // mark if this net is routed successfully
	FLUIDIC_RESULT fluid_result;
	while( !p.empty() ){
		if( p.size() > MAXHEAPSIZE ){
			// jump out to rip up others
			cerr<<"heap size exceed"<<endl;
			break;
			//report_exit("Heap exceed!");
		}
		// if there are too much conflict, ripup and reroute
		if( conflict_net.total > MAXCFLT ){
			cerr<<"too much conflict"<<endl;
			break;
		}
		// if there is some net causing too much conflict
		int mid = conflict_net.max_id;
		if( conflict_net.conflict_count[mid] > MAX_SINGLE_CFLT ){
			cerr<<"too much conflict caused by "<<mid<<endl;
			break;
		}
		// get wave_front and propagate its neighbour
		//p.sort();
#ifdef OUTPUT
		if( which == 1 ){
		cout<<"------------------------------------------------"<<endl;
		cout<<"[before pop]"<<endl;
		output_heap(p);
		}
#endif 
		current = p.top();
		p.pop();
#ifdef OUTPUT
		if( which == 1 ){
		cout<<"[after pop]"<<endl;
		cout<<"Pop "         <<current->pt
		    <<" at time "    <<current->time
		    <<", queue size="<<p.size()<<endl;
		output_heap(p);
//		p.sort();
//		output_heap(p);
		}
#endif 

		// sink reached, but need to check whether it stays
		// there will block others
		if( current->pt == dst ) {
			int reach_t = current->time;
			bool fail = false;
			// see whether it can stay in destination
			// TODO: also need to check electrode constraint here!!
			for (int i = reach_t+1; i <= T; i++) {
				fluid_result = fluidic_check(which,
						pin_idx, current->pt,
						i,result,conflict_net);
				if( fluid_result == VIOLATE ){
					//assert(conflict_netid != which);
					//conflict_net.increment(
					//conflict_netid);
					fail = true;
					break;
				}

				bool not_elect_violate=electrode_check(
						which,pin_idx,
						current->pt,current->pt,i,
						result,conflict_net,0);
				if( !not_elect_violate ){
					fail = true;
					break;
				}
			}
			if( !fail ){// safely enter destination and stay
				cout<<"Find "<<dst
				    <<" at time "<<current->time<<"!"<<endl;
				if( reach_t > max_t ) max_t = reach_t;
				success = true;
				break;
			}// otherwise, continue to search
		}
		
		// did not reach sink...propagate this gridpoint
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
	//output_result(result);
	p.free();
	return true;

}

// given a net with index `which' , route all its subnets
// 2pin net: 1 subnet
// 3pin net: 2 subnet
bool Router::route_net(int which,RouteResult &result) {
	cout<<"** Routing net["<<which<<"] **"<<endl;
	Net * pNet = &pProb->net[which];
	output_netinfo(pNet);
	bool success;
	ConflictSet conflict_net(netcount);
	
	do{
		memset(visited,0,sizeof(visited));
		if( pNet->numPin == 2 )
			success=route_2pin(which,result,conflict_net);
		else
			success=route_3pin(which,result,conflict_net);

		if( success == false ){
			// first net can not be route: routing failed
			if(netorder[0]==which) 
				return false;
			cerr<<"** Error: route net["<<which
			    <<"] failed,ripup-reroute....."<<endl;
			bool ret;
			ret = ripup_reroute(which,result,conflict_net);
			if( ret == false ) return false;
			output_netorder(netorder,netcount);
			//cout<<"after rip"<<endl;
			//output_result(result);
			cout<<"re-route net "<<which<<endl;
			last_ripper_id = which;
		}
	}while(success == false);

	return true;
}

bool Router::route_2pin(int which,RouteResult & result,
		ConflictSet & conflict_net){
	Net * pNet = &pProb->net[which];
	Point src=pNet->pin[0].pt;
	Point dst = get_netdst_pt(which);
	return route_subnet(src,dst,which,0,
			result,conflict_net);
}

bool Router::route_3pin(int which,RouteResult & result,
		ConflictSet & conflict_net){
	Net * pNet = &pProb->net[which];
	Point src,dst;
	bool ret;

	// decompose to 2 subnet
	// first compute a merge point...
	// now just use the sink as the merge point...
	int dist1 = MHT(pNet->pin[0].pt,pNet->pin[2].pt);
	int dist2 = MHT(pNet->pin[1].pt,pNet->pin[2].pt);
	int a=0,b=1;
	if( dist1 > dist2 ){
		swap(a,b);
	}
	
	// route 1st subnet there
	src = pNet->pin[a].pt;
	dst = get_netdst_pt(which);
	ret = route_subnet(src,dst,which,a,
		result,conflict_net);
	if( ret == false ) return false;
	
	// route 2nd subnet there
	src = pNet->pin[b].pt;
	dst = get_netdst_pt(which);
	ret = route_subnet(src,dst,which,b,
		result,conflict_net);
	if( ret == false ) return false;

	// modified the result to a valid one
	// 1st droplet moved to merge, then move back to sink
	PtVector & pin_patha = result.path[which].pin_route[a];
	PtVector & pin_pathb = result.path[which].pin_route[b];
	int merge_time = result.path[which].reach_time[b]-1;
	pin_patha[merge_time] = pin_pathb[merge_time];
	result.path[which].merge_time=merge_time;
	
	// route the merged droplet to final sink
	return true;
}

// for a given net `which', rip up the net causing most conflict
// also re-order the netorder
// POSTCONDITION: the net order will be changed
bool Router::ripup_reroute(int which,RouteResult & result,
		ConflictSet &conflict_net){
	// cancel the route result of some conflict net
	// now use the most conflict net=`max_id'
	int max_id = conflict_net.max_id;
	assert(max_id>=0);

	// what about for 3-pin net that is causing constraint on itself?
	assert(max_id!=which);

	// check if max_id is the net that rip `which' previously
	if(max_id == last_ripper_id){
		cout<<"deadlock...break using the 2nd largest"<<endl;
		// find the 2nd largest
		int max_count=-1;
		max_id=-1;
		for (int i = 0; i < conflict_net.net_num; i++) {
			//cout<<"confclit "<<i<<"="
			//    <<conflict_net.conflict_count[i]<<endl;
			if( max_count < conflict_net.conflict_count[i] &&
			    i != conflict_net.max_id ){
				max_count = conflict_net.conflict_count[i];
				max_id = i;
			}
		}
	}
	
	if( max_id == -1 ){ // there is no 2nd largest...
		return false;
	}

	cout<<"** ripping net "<<max_id<<",conflict count="
	    <<conflict_net.conflict_count[max_id]
	    <<" last ripper = "<<last_ripper_id<<" **"<<endl;

	result.path[max_id].clear(); // cancel the routed path

	// clear the conflict result of this net
	conflict_net =  ConflictSet(conflict_net.net_num);

	// find there index in netorder
	int max_id_inorder,which_id_inorder;
	for(int i = 0; i < netcount; i++) {
		if( netorder[i] == max_id) max_id_inorder = i;
		if( netorder[i] == which) which_id_inorder = i;
	}

	// re-push into queue(so that re route `max_id')
	nets.push_front(max_id);

	// re-order route order
	IntVector temp(netorder,netorder+netcount);
	temp.erase(temp.begin()+max_id_inorder);          //remove net=max
	temp.insert(temp.begin()+which_id_inorder,max_id);//insert after which
	copy(temp.begin(),temp.end(),netorder);

	// remove the edges caused by this net in the graph
	// now just clear all graph, and re-add constraint
	destroy_graph();
	allocate_graph();
	for (int i=0;i<netcount && netorder[i]!=which; i++) {
		int checking_idx = netorder[i];
		//if( i == max_id_inorder ) continue; // do not add this net
		const NetRoute & route = result.path[checking_idx];
		for(int j=0;j<route.num_pin-1;j++){
			const PtVector & pin_path = 
				result.path[checking_idx].pin_route[j];
			// reconstruct the graph
			update_graph(checking_idx,j,pin_path,result);
		}
	}

	//`which' moved one place before in netorder
	//return which_id_inorder-1;
	return true;
}

// given a grid point `gp_from' during propagation of routing net `which',
// generate its neighbours push into heap if satisfies constraint.
// dst : the sink point, 
// result: store the routing result
// p: heap
// possible_nets : keep info. of the conflicting net
// return false it NO cells pushed
bool Router::propagate_nbrs(int which, int pin_idx,GridPoint * gp_from,
			    Point & dst,RouteResult & result,
			    GP_HEAP & p,ConflictSet &possible_nets){
	bool has_pushed=false;
	Point from_pt = gp_from->pt;
	// get its neighbours( PROBLEM: can it be back? )
	PtVector nbr = get_neighbour(from_pt);
	const int t = (gp_from->time + 1);

	// enqueue neighbours
	GridPoint * par_par = gp_from->parent; 
	for(size_t i=0;i<nbr.size();i++){
		int x=nbr[i].x,y=nbr[i].y;
		// 0.gp_from pt should be avoided(why?)
		// 1.parent should not be propagated again
		// 2.check if there is blockage 
		// 3.forbid circular move (1->2...->1) unless it is stalling
		// IMPORTANT: shall we check the backward move?
		// there should be some rule to drop GridPoint
		//if( nbr[i] != pt && nbr[i] == from_pt ) continue;
		if( blockage[x][y] == BLOCK ) continue;
		if( (par_par != NULL) && 
		     (nbr[i] == par_par->pt) &&
			(nbr[i] != from_pt) )
			continue;
		if( visited[x][y][t] == 1 ) continue; 
		visited[x][y][t] = 1;
		// calculate its weight
		Point moving_to(x,y);
		int f_pen=0,e_pen=0,bending=gp_from->bend;
		//int conflict_netid;

		// fluidic constraint check
		FLUIDIC_RESULT fluid_result=fluidic_check(which,pin_idx,
				moving_to,t,result,possible_nets);
		if( fluid_result == SAMENET ){
			// multipin net
			report_exit("fluid_result == SAMENET");
		} 
		else if( fluid_result == VIOLATE ){
			//assert(conflict_netid != which);
			//possible_nets.increment(conflict_netid);
			//cout<<"net "<<which<<" fluidic violation:"
			//    <<from_pt<<"->"<<moving_to<<" time="<<t<<endl;
			continue;
		}
		//else if (fluid_result == SRC_VIOLATE)
		//	continue;

		// electro constraint check
		bool not_elect_violate=electrode_check(which,pin_idx,
			       	moving_to,from_pt,t,result,possible_nets,0);
		if( !not_elect_violate ){
			//cout<<"net "<<which<<" electrode violation:"
			//   <<from_pt<<"->"<<moving_to<<endl;
			continue;
		}

		// bending update
		if( par_par != NULL &&
		    check_bending(moving_to,par_par->pt) == true )
			bending++;

		GridPoint *nbpt = new GridPoint(
				moving_to,gp_from,
				t, bending,
				f_pen, e_pen,
				gp_from->stalling,
				MHT(moving_to,dst));
		p.push(nbpt);
		has_pushed = true;
#ifdef DEBUG
		cout<<"\tAdd t="<<t<<":"<<*nbpt<<endl;
#endif
	}// end of enqueue neighbours
	return has_pushed;
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
	/*
	cout<<" propagated = "<<GridPoint::counter
	    <<" found at = "<<current->order<<endl;
	    */
	PtVector & pin_path = result.path[which].pin_route[pin_idx];
	pin_path.clear();	// for 3-pin net re-route, may need to clear it
	Point dst = current->pt;
	result.path[which].reach_time[pin_idx] = current->time;
	while( current != NULL ){// trace back from dest to src
		pin_path.insert(pin_path.begin(),current->pt);
		if( current->parent == NULL ) break;
		current = current->parent;
	}

	// IMPORATANT: for waste disposal point
	// DO NOT need to generate STAY result
	// e.g. reaches at t=13, then disppear from t=14
	if( dst != chip.WAT ) {
		// that some droplet may reach the destination earlier than the
		// timing constraint. fill later time with its final position
		for(int i=pin_path.size();i<=this->T;i++){
			pin_path.push_back(dst); // dst is destination pt here
		}
	}

	// add the constraint into ALL graph
	update_graph(which,pin_idx,pin_path,result);

	// output result
	//output_result(result);
}

// given subnet `pin_idx' in a net `which' , and its route result
// re-construct the constraint graph
void Router::update_graph(int which,int pin_idx,
		const PtVector & pin_path,
		const RouteResult & result){
	// we need to update all the coloring status 
	// for every time step until it reaches the dest, 
	// however, do not add the stalling frame!
	ConflictSet dummy(netcount);
	for (size_t i = 1; i < pin_path.size(); i++) {
		// i is time
		Point p = pin_path[i],q=pin_path[i-1];
		DIRECTION dir = pt_relative_pos(q,p);
		if( dir != STAY ){
			//int t = current->time;
			bool success = electrode_check(which,pin_idx,
					p,q,i,result,dummy,1);
			// IMPORTANT: return value must be true here!
			assert(success == true);
		}
	}
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
// REMEMBER to ensure the validity of the point(be in chip)
PtVector Router::get_neighbour(const Point & pt){
	PtVector s;
	for(int i=0;i<4;i++){
		Point p(pt.x+dx[i],pt.y+dy[i]);
		if( in_grid(p) )
			s.push_back(p);
	}
	s.push_back(pt); // add the same position for stalling
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
	// test: set predefined netorder here
	/*
	int predefine[]={2,0,1,3};
	std::copy(predefine,predefine+4,netorder);
	*/
}

// outputs the net's all pins
void Router::output_netinfo(Net *pNet){
	for(int i=0;i<pNet->numPin;i++)// output Net info
		cout<<"\t"<<"pin "<<i<<":"<<pNet->pin[i].pt<<endl;
}

// outputs the elements in `netorder'
void Router::output_netorder(int *netorder,int netcount){
	cout<<"net order: [ ";
	for(int i=0;i<netcount;i++) cout<<netorder[i]<<" ";
	cout<<"]"<<endl;
}

// control=0:test / control=1:save
// droplet d1 is `pin_idx' subnet of net `which'
// `d1' is moving to a point `pt' at time `t', 
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
#define check_result(c_net,net_id) \
	if(add_result==false){\
		(c_net).increment(net_id);\
		return false;\
	}

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
		// how to know what nets are affecting it? 
		if( add_result == false ) {
			//cout<<"reached type 1"<<endl;
			return false;
		}
	}

	// let another droplet be `d2'
	for (int i = 0; i<netcount ; i++) {
		int checking_idx = netorder[i];
		const NetRoute & route = result.path[checking_idx];
		// for each subnet(at most 2)
		for (int j = 0; j<route.num_pin-1; j++) {
			// do not check it self!
			if( checking_idx == which && j == pin_idx ) continue;

			const PtVector & pin = route.pin_route[j];
			// pin[t] is the location at time t(activated at t-1)
			// IMOPRTANT:some droplet may disappear(waste disposal)
			if( t >= (int)pin.size() ) break;
			// IMPORTANT: 
			// if two droplet's sharing same activating row/column
			// NO need to check the constraint!( my assumption )
			if( pt.x == pin[t].x || pt.y == pin[t].y ) continue;

			// Type 2: check if net-which+other-net affect d2
			if( dir1 != STAY ){
				add_result=check_droplet_conflict(
						parent_pt,pt,
						pin[t-1],pin[t],
						p_graph,t,control);
				check_result(conflict_net,checking_idx);
			}
			// Type 3: check if d2+other-net affect net-which
			DIRECTION dir2 = pt_relative_pos(pin[t-1],pin[t]);
			if( dir2 != STAY ){
				add_result=check_droplet_conflict(
						pin[t-1],pin[t],
						parent_pt,pt,
						p_graph,t,control);
				check_result(conflict_net,checking_idx);
			}
		} // end of for j
		// after the checking of the same net, break the for loop
		if( checking_idx==which ) break; 
	} // end of for i

	// check 3-pin net here
	/*
	if( get_pinnum(which) == 3 && pin_idx == 1 ){
		// if it is a merging step, no need to add constraint
	}
	*/

	// no electro constraint violation!
	return true;
#undef check_result
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
	PtVector pts,cells;
	const int xs[]={T.x-1,T.x,T.x+1};
	for (int i = 0; i < 3; i++) 
		if( xs[i] >= 0 && xs[i] < W ) 
			cells.push_back( Point(xs[i],hline) );
	DIRECTION dir = pt_relative_pos(S,T);

	// if the line not intersect with 5x5 bounding box(BB) of T
	int delta = hline - T.y;
	if( ABS(delta) > 2 ) return pts; // empty
	if( ABS(delta) <=1 ){// intersect with 3x3 BB
		pts = cells;
		switch( dir ){
			case RIGHT: 
				if( S.x+1 < W )
					pts.push_back(Point(S.x+1,S.y));
				break;
			case LEFT:
				if( S.x-1 >= 0 )
					pts.push_back(Point(S.x-1,S.y));
				break;
			default:break; // stalling or moving vertically
		}
	}
	else if( (dir == DOWN   && hline == S.y-1) || 
		 (dir == UP && hline == S.y+1)) {// ABS(delta) = 2
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
	PtVector pts,cells;
	const int ys[]={T.y-1,T.y,T.y+1};
	for (int i = 0; i < 3; i++) 
		if( ys[i] >= 0 && ys[i] < H ) 
			cells.push_back( Point(vline,ys[i]) );
	DIRECTION dir = pt_relative_pos(S,T);

	// if the line not intersect with 5x5 bounding box(BB) of T
	int delta = vline - T.x;
	if( ABS(delta) > 2 ) return pts; // empty
	if( ABS(delta) <=1 ){// intersect with 3x3 BB
		pts = cells;
		switch( dir ){
			case DOWN: 
				if( S.y-1 >= 0 )
					pts.push_back(Point(vline,S.y-1));
				break;
			case UP:
				if( S.y+1 < H )
					pts.push_back(Point(vline,S.y+1));
				break;
			default:break; // stalling or moving vertically
		}
	}
	else if( (dir == RIGHT  && vline == S.x+1) || 
		 (dir == LEFT && vline == S.x-1)) {// ABS(delta) = 2
		// intersects with left or right boundary of 5x5 BB
		pts = cells;
	}
	return pts;
}

// given a vertical or horizontal line, a droplet's movement 
// return a set of cells the line intersects with
// (3x3 BB or 4x3 BB)
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
		// if pts is in blockage, do not check it
		int x = pts[i].x, y = pts[i].y;
		//if( blockage[x][y] == BLOCK ) continue;
		// get the information of cell i in graph
		if( ntype == ROW ){
			// get the node of col pts[i].x
			GNode col_node(COL,x);
			GNode row_node(ROW,lineid);
			if( p_graph->get_node_color(col_node) != G ){
				result = p_graph->add_edge_color(
						col_node,row_node,SAME);
				if(result == false) return false;
			}
		}	
		else{// COL
			GNode col_node(COL,lineid);
			GNode row_node(ROW,y);
			if( p_graph->get_node_color(row_node) != G ){
				result = p_graph->add_edge_color(
						col_node,row_node,SAME);
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
		const RouteResult & result,ConflictSet & conflict_set){
	// for each routed net(including partial), 
	// check if the current routing net(which) violate fluidic rule
	// we have known the previous routed net 
	// from netorder[0] to netorder[i]!=which
	// t's range: [0..T]
	assert( (t <= this->T) && (t >= 0) );
	// for all the net routed before which
	int i;
	for(i=0;i<netcount && netorder[i]!=which;i++){
		int checking_idx = netorder[i];
		// NOTE: if two nets have same dest, they are going to
		// the waste disposal point
		//if( net_same_dest(which,checking_idx) ) return SAMEDEST;
		const NetRoute & route = result.path[checking_idx];
		// for each subnet
		for (int j = 0; j < route.num_pin-1; j++) {
			const PtVector & path = route.pin_route[j];
			// static fluidic check
			if( static_violate(pt,t) || 
		            dynamic_violate(pt,t) ){
				if( pProb->net[i].pin[1].pt != this->chip.WAT )
					conflict_set.increment(checking_idx);
				return VIOLATE;
			}
		} // end of for j
	} // end of for i, now i == which or i>=netcount
	
	// IMPORTANT: check whether it hits unrouted net
	/*
	for(i=i+1;i<netcount;i++){
		int checking_idx = netorder[i];
		Net * pNet = &(pProb->net[checking_idx]);
		for (int j = 0; j < pNet->numPin-1; j++) {
			Point src = pNet->pin[j].pt;
			if( !(abs(pt.x - src.x) >=2 || 
			      abs(pt.y - src.y) >=2) )
				return SRC_VIOLATE;
		}
	}
	*/

	// check for 3-pin net
	/*
	const NetRoute & route = result.path[which];
	if( route.num_pin == 3 ){
		const PtVector & path = route.pin_route[0];
		if( static_violate(pt,t) || dynamic_violate(pt,t) ){
			return SAMENET;  // they should merge
		}
	}
	*/
	return SAFE;
}

// solves the command line argument
ResultVector Router::solve_cmdline(){
	// tosolve is not given in cmdline
	if( this->tosolve == -1 ) 
		route_result = solve_all();
	else
		route_result.push_back(solve_subproblem(tosolve));
	return route_result;
}
