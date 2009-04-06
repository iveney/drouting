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
		int which = netorder[i];
		route_net(which,result);
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

vector<RouteResult> Router::solve_all(){
	for(int i=0;i<chip.nSubProblem;i++)
		solve_subproblem(i);
	return route_result;
}

void Router::route_net(int which,RouteResult & result){
	cout<<"** Routing net["<<which<<"] **"<<endl;
	GridPoint *current;
	Net * pNet = &pProb->net[which];
	output_netinfo(pNet);

	// do Lee's propagation,handles 2-pin net only currently
	int numPin = pNet->numPin;

	//if( numPin == 3 ) {} // handle three pin net
	Point S = pNet->pin[0].pt; // source
	Point T = pNet->pin[1].pt; // sink

	// initialize the heap
	GP_HEAP p;

	// start time = 0, source point = S, no parent
	GridPoint *src = new GridPoint(S,NULL); 
	src->distance = MHT(S,T);
	p.push(src); // put the source point into heap

	int t=0;               // current time step
	bool success = false;  // mark whether this net is routed successfully
	while( !p.empty() ){
		// get wave_front and propagate its neighbour
		//p.sort();
#ifdef DEBUG
		cout<<"------------------------------------------------------------"<<endl;
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

		// find the sink!
		if( current->pt == T ) {
			cout<<"Find "<<T<<" at time "<<current->time<<"!"<<endl;
			success = true;
			break;
		}
		
		t = current->time+1;
		if( t > this->T ){ // timing constraint violated
			// just drop this node
			if( p.size() != 0 ) 
				continue;
			// error, can not find route
			else{
				cerr<<"Exceed route time!"<<endl;
				// reroute();  // try rip-up and re-route?
				success = false;
				break;
			}
		}

		// stall at the same position, stall for 1 time step
		GridPoint *same = new GridPoint( current->pt,current,
				t,current->bend,  
				current->fluidic,
				current->electro, 
				current->stalling+STALL_PENALTY,
				current->distance );
		p.push(same);
#ifdef DEBUG
		cout<<"\tAdd:"<<*same<<endl;
#endif

		// get its neighbours( PROBLEM: can it be back? )
		vector<Point> nbr = get_neighbour(current->pt);
		vector<Point>::iterator iter;

		// enqueue neighbours
		GridPoint * par_par = current->parent;
		for(iter = nbr.begin();iter!=nbr.end();iter++){
			int x=(*iter).x,y=(*iter).y;
			// 0.current pt should be avoided
			// 1.parent should not be propagated again
			// 2.check if there is blockage 
			// 3.forbid circular move (1->2...->1)
			if( !blockage[x][y] &&
		             (*iter) != current->pt ){ 
				if(par_par != NULL && 
				  (*iter) == par_par->pt) 
					continue;
				// calculate its weight
				Point tmp(x,y);
				int f_pen=0,e_pen=0,bending=current->bend;
				int fluidic_result=fluidic_check(which,tmp,t,result);
				bool electro_result=electrode_check( tmp );

				//TODO: do not add this, but just drop it
				// fluidic constraint
				if( fluidic_result != -1 ){
					continue;
					//f_pen = FLUID_PENALTY;
				}

				// electro constraint
				if( !electro_result ){
					continue;
					//e_pen = ELECT_PENALTY;
				}

				// check bending
				if( par_par != NULL &&
				    check_bending(tmp,par_par->pt) == true )
					bending++;

				GridPoint *nbpt = new GridPoint(
						tmp,current,
						t, bending,
						f_pen, e_pen,
						current->stalling,
						MHT(tmp,T));
				p.push(nbpt);
#ifdef DEBUG
				cout<<"\tAdd:"<<*nbpt<<endl;
#endif
			}
		}// end of enqueue neighbours
	}// end of propagate
	
	// failed to find path, TODO: reroute?
	if( success == false ){
		report_exit("Error: failed to find path");
	}
#ifdef DEBUG
	else{ cout<<"Success - start to backtrack"<<endl; }
#endif
	//////////////////////////////////////////////////////////////////
	// backtrack phase, stores results to RoutingResult
	// output format:(e.g.)
	// net[0]:20
	//     0: (1,3)
	//     1: (2,3)
	//     2: ...
	//     20:(21,3)
	//
	cout<<"net["<<which<<"]:"<<this->T<<endl;
	vector<Point> & net_path = result.path[which];
	while( current != NULL ){
		//cout<<"\t"<<current->time<<":"<<current->pt<<endl;
		net_path.insert(net_path.begin(),current->pt);
		current = current->parent;
	}

	// note that some droplet may reach the destination early than the
	// timing constraint. fill all later time with its final position
	for(int i=net_path.size();i<=this->T;i++){
		net_path.push_back(T); // T is destination pt here
	}

	// output result
	for(size_t i=0;i<net_path.size();i++){
		cout<<"\t"<<i<<":"
		    <<net_path[i]<<endl;
	}

	p.free();
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
vector<Point> Router::get_neighbour(const Point & pt){
	vector<Point> s;
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
int Router::fluidic_check(int which, const Point & pt,int t,
		const RouteResult & result){
	// for each routed net, 
	// check if the current routing net(which) violate fluidic rule
	// we have known the previous routed net 
	// from netorder[0] to netorder[i]!=which
	// t's range: [0..T]
	if(t > this->T || t < 0)
		cout<<"which="<<which<<" Point="<<pt<<" t="<<t<<endl;
	assert( (t <= this->T) && (t >= 0) );
	for(int i=0;i<netcount && netorder[i] != which;i++){
		int checking_idx = netorder[i];
		const vector<Point> & path = result.path[checking_idx];
		// static fluidic check
		if( !(abs(pt.x - path[t].x) >=2 ||
		      abs(pt.y - path[t].y) >=2) )
			return i;
		// dynamic fluidic check
		if ( !(abs(pt.x - path[t-1].x) >=2 ||
		       abs(pt.y - path[t-1].y) >=2) )
			return i;
	}
	return -1;
}

vector<RouteResult> Router::solve_cmdline(){
	// tosolve is not given in cmdline
	if( this->tosolve == -1 ) 
		route_result = solve_all();
	else
		route_result.push_back(solve_subproblem(tosolve));
	return route_result;
}
