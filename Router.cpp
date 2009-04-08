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

Subproblem * Router::pProb=NULL;

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
	read=true;
}

RouteResult Router::solve_subproblem(int prob_idx){
	if( read == false ) report_exit("Must read input first!");
	cout<<"--- Solving subproblem ["<<prob_idx<<"] ---"<<endl;
	
	// solve subproblem `idx'
	pProb = &chip.prob[prob_idx];
	
	// the result to return
	RouteResult result(this->T,this->M,this->N,*(this->pProb));

	// sort : decide net order
	netcount = pProb->nNet;
	sort_net(pProb,netorder);
	N=chip.N;
	M=chip.M;
	T=chip.T;

	output_netorder(netorder,netcount);

	// generate blockage bitmap
	init_block(pProb);

	// start to route each net according to sorted order
	for(int i=0;i<netcount;i++){
		ConflictSet conflict_net(netcount);
		int which = netorder[i];
		bool success = route_net(which,result,conflict_net);
		if( success == false ){
			if( i == 0 ){
				// first net can not be routed...
				report_exit("Error: route failed!");
			}
			cerr<<"Error: route net "<<which
			    <<" failed ,try ripup-reroute!"<<endl;
			i=ripup_reroute(which,result,conflict_net)-1;
			output_netorder(netorder,netcount);
		}
	}

	// finally, output result
	cout<<"Subproblem "<<prob_idx<<" solved!"<<endl;
	for (int i = 0; i < netcount; i++) {
		cout<<"net["<<i<<"]:"<<this->T<<endl;
		PtVector & net_path = result.path[i];
		for(size_t j=0;j<net_path.size();j++){
			cout<<"\t"<<j<<":"
				<<net_path[j]<<endl;
		}
	}

	return result ;
}

void Router::init_block(Subproblem *p){
	memset(blockage,0,sizeof(blockage));
	int i,x,y;
	for(i=0;i<p->nBlock;i++) {
		Block b = p->block[i];
		for(x=b.pt[0].x;x<=b.pt[1].x;x++)
			for(y=b.pt[0].y;y<=b.pt[1].y;y++)
				blockage[x][y]=1;
	}
}

ResultVector Router::solve_all(){
	for(int i=0;i<chip.nSubProblem;i++)
		solve_subproblem(i);
	return route_result;
}

bool Router::route_net(int which,RouteResult &result,ConflictSet &conflict_net){
	cout<<"** Routing net["<<which<<"] **"<<endl;
	GridPoint *current;
	Net * pNet = &pProb->net[which];
	output_netinfo(pNet);

	// do Lee's propagation,handles 2-pin net only currently
	Point src = pNet->pin[0].pt; // source
	Point dst = pNet->pin[1].pt; // sink
	int numPin = pNet->numPin;
	if( numPin == 3 ) {// handle three pin net
		dst = pNet->pin[2].pt; // sink
	}       

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
			for (int i = reach_t+1; i <= T; i++) {
				fluid_result = fluidic_check(which,
				current->pt,i,result,conflict_net);
				if( fluid_result == VIOLATE ){
					fail = true;
					break;
				}
			}
			if( !fail ){
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
		// NOTE: also need to check fluidic constraint here
		// may block routed net's path
		GridPoint *same = new GridPoint( current->pt,
				current, t,current->bend,  
				current->fluidic, current->electro, 
				current->stalling+STALL_PENALTY,
				current->distance );
		fluid_result=fluidic_check(which,
			same->pt,same->time,result,conflict_net);
		if( fluid_result == SAMENET ){
			// multipin net
		}else if( fluid_result != VIOLATE){
			p.push(same);
#ifdef DEBUG
			cout<<"\tAdd:"<<*same<<endl;
#endif
		}

		// propagate current point
		propagate_nbrs(which,current,dst,result,p,conflict_net);
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
	// backtrack phase, stores results to RouteResult
	backtrack(which,current,result);
	p.free();
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
	result.path[last].clear();

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
void Router::propagate_nbrs(int which,GridPoint * current,
			    Point & dst,RouteResult & result,
			    GP_HEAP & p,ConflictSet &conflict_net){
	// get its neighbours( PROBLEM: can it be back? )
	PtVector nbr = get_neighbour(current->pt);
	PtVector::iterator iter;
	const int t = (current->time + 1);

	// enqueue neighbours
	GridPoint * par_par = current->parent; 
	for(iter = nbr.begin();iter!=nbr.end();iter++){
		int x=(*iter).x,y=(*iter).y;
		// 0.current pt should be avoided
		// 1.parent should not be propagated again
		// 2.check if there is blockage 
		// 3.forbid circular move (1->2...->1)
		if( (*iter) == current->pt ) continue;
		if( blockage[x][y] ) continue;
		if( (par_par != NULL) && 
		    (*iter == par_par->pt) )
			continue;
		// calculate its weight
		Point tmp(x,y);
		int f_pen=0,e_pen=0,bending=current->bend;
		FLUIDIC_RESULT fluid_result=fluidic_check(which,
				tmp,t,result,conflict_net);
		bool elect_violate=electrode_check( tmp );

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
void Router::backtrack(int which,GridPoint *current,
		RouteResult & result){
	// output format:(e.g.)
	// net[0]:20
	//     0: (1,3)
	//     1: (2,3)
	//     2: ...
	//     20:(21,3)
	//
	PtVector & net_path = result.path[which];
	Point dst = current->pt;
	while( current != NULL ){
		//cout<<"\t"<<current->time<<":"<<current->pt<<endl;
		net_path.insert(net_path.begin(),current->pt);
		current = current->parent;
	}

	// note that some droplet may reach the destination early than the
	// timing constraint. fill all later time with its final position
	for(int i=net_path.size();i<=this->T;i++){
		net_path.push_back(dst); // dst is destination pt here
	}

	// output result
	/*
	cout<<"net["<<which<<"]:"<<this->T<<endl;
	for(size_t i=0;i<net_path.size();i++){
		cout<<"\t"<<i<<":"
			<<net_path[i]<<endl;
	}
	*/
}

// just print out what is in the heap
void Router::output_heap(const GP_HEAP & h){
	for(int i=0;i<h.size();i++) {
		cout<<i<<":"<<*(h.c[i])<<endl;
	}
}

// test if a point is in the chip array
bool Router::in_grid(const Point & pt){
	if( pt.x >=0 && pt.x <N && 
	    pt.y >=0 && pt.y <M) return true;
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
	int N=pProb->nNet;
	qsort(netorder,N,sizeof(int),cmp_net);
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

// perform electrode constraint check
bool Router::electrode_check(const Point & pt){
	// use DFS to check : 2-color
	
	return true;
}

// for a net `which' at location `pt' at time `t', 
// perform fluidic constraint check
// if successful return 0 
// else return the conflicting net
FLUIDIC_RESULT Router::fluidic_check(int which, const Point & pt,int t,
		const RouteResult & result,ConflictSet & conflict_net){
	// for each routed net, 
	// check if the current routing net(which) violate fluidic rule
	// we have known the previous routed net 
	// from netorder[0] to netorder[i]!=which
	// t's range: [0..T]
	//if(t > this->T || t < 0)
	//	cout<<"which="<<which<<" Point="<<pt<<" t="<<t<<endl;
	assert( (t <= this->T) && (t >= 0) );
	for(int i=0;i<netcount && netorder[i] != which;i++){
		int checking_idx = netorder[i];
		const PtVector & path = result.path[checking_idx];
		// TODO: detect 3-pin net merge
		// static fluidic check
		if( !(abs(pt.x - path[t].x) >=2 ||
		      abs(pt.y - path[t].y) >=2) ){
			//cout<<"static:net "<<checking_idx<<"at " <<path[t]<<endl;
			conflict_net.insert(checking_idx);
			return VIOLATE;
		}
		// dynamic fluidic check
		if ( !(abs(pt.x - path[t-1].x) >=2 ||
		       abs(pt.y - path[t-1].y) >=2) ){
			//cout<<"dynamic:net "<<checking_idx<<"at " <<path[t]<<endl;
			conflict_net.insert(checking_idx);
			return VIOLATE;
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
