#include <iostream>
#include <stdlib.h>
#include "header.h"
#include "GridPoint.h"
#include "Router.h"
#include "parser.h"
#include "util.h"
using std::cout;
using std::cerr;
using std::endl;

int dx[]={-1,1,0,0,0};
int dy[]={0,0,1,-1,0};

void Router::read_file(int argc, char * argv[]){
	FILE * f;
	if(argc<3)
		report_exit("Usage ./main filename subproblem\n");

	const char * filename = argv[1];
	tosolve = atoi(argv[2]);
	if( (f = fopen(filename,"r")) == NULL )
		report_exit("open file error\n");

	parse(f,&chip); // now `chip' has store subproblems
	fclose(f);
	read=true;
}

RouteResult Router::solve_subproblem(int prob_idx){
	if( read == false ) report_exit("Must read input first!");
	cout<<"--- Solving subproblem ["<<prob_idx<<"] ---"<<endl;
	
	// the result to return
	RouteResult result;

	// solve subproblem `idx'
	pProb = &chip.prob[prob_idx];
	
	// sort : decide net order
	netcount = pProb->nNet;
	sort_net(pProb,netorder);

	// generate blockage bitmap
	init_block(pProb);

	// start to route each net according to sorted order
	for(int i=0;i<netcount;i++){
		int which = netorder[i];
		route_net(which);
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

void Router::route_net(int which){
	cout<<"** Routing net["<<which<<"] **"<<endl;
	GridPoint *current;
	Net * pNet = &pProb->net[which];

	// do Lee's propagation,handles 2-pin net only currently
	int numPin = pNet->numPin;

	//if( numPin == 3 ) {} // handle three pin net
	Point S = pNet->pin[0].pt; // source
	Point T = pNet->pin[1].pt; // sink

	for(int i=0;i<numPin;i++)// output Net infor
		cout<<"\t"<<"pin "<<i<<":"<<pNet->pin[i].pt<<endl;

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
#ifdef DEBUG
		cout<<"------------------------------------------------------------"<<endl;
		cout<<"[before pop]"<<endl;
		//p.sort();
		for(int i=0;i<p.size();i++)
			cout<<i<<" "<<(p.c[i])->pt<<" t="<<p.c[i]->time<<" w="<<p.c[i]->weight<<",order="<<p.c[i]->order<<endl;
#endif 
		current = p.top();
		p.pop();
#ifdef DEBUG
		cout<<"[after pop]"<<endl;
		//p.sort();
		for(int i=0;i<p.size();i++)
			cout<<i<<" "<<(p.c[i])->pt<<" t="<<p.c[i]->time<<" w="<<p.c[i]->weight<<",order="<<p.c[i]->order<<endl;
#endif 
		cout<<"Pop "<<current->pt<<" at time "<<current->time<<", queue size="<<p.size()<<endl;
		cout<<"t="<<t<<",Propagating"<<current->pt<<endl;

		t = current->time+1;
		if( t > MAXTIME+1 ){ // timing constraint violated
			if( p.size() != 0 )
				continue;
			else{
				cerr<<"Exceed route time!"<<endl;
				// reroute();  // try rip-up and re-route?
				success = false;
				break;
			}
		}

		// find the sink, horray!
		if( current->pt == T ) {
			cout<<"Find "<<T<<"!\n";
			success = true;
			break;
		}

		// stall at the same position, stall for 1 time step
		GridPoint *same = new GridPoint( current->pt,current,
				t,current->bend,  current->fluidic,
				current->electro, current->stalling+STALL_PENALTY,
				current->distance );
		p.push(same);
#ifdef DEBUG
		cout<<"\tStalling Point "<<same->pt<<" pushed. w="<<same->weight<<
			", parent = "<<same->parent->pt<<endl;
#endif

		// get its neighbours( PROBLEM: can it be back? )
		vector<Point> nbr = get_neighbour(current->pt);
		vector<Point>::iterator iter;

		// enqueue neighbours
		for(iter = nbr.begin();iter!=nbr.end();iter++){
			int x=(*iter).x,y=(*iter).y;
			// 0.current pt should be avoided
			// 1.parent should not be propagated again
			// 2.check if there is blockage 
			// 3.forbid circular move (1->2...->1)
			if( !blockage[x][y] &&
					(*iter) != current->pt ){ 
				if(current->parent != NULL && 
						(*iter) == current->parent->pt) 
					continue;
				// calculate its weight
				Point tmp(x,y);
				int f_pen=0,e_pen=0,bending=current->bend;
				int fluidic_result = fluidic_check( which,tmp,t );
				bool electro_result = electrode_check( tmp );

				// fluidic constraint
				if( fluidic_result != 0 )
					f_pen = FLUID_PENALTY;

				// electro constraint
				if( !electro_result )
					e_pen = ELECT_PENALTY;

				// check bending
				if( current->parent != NULL &&
						check_bending(tmp,current->parent->pt) == true )
					bending++;

				GridPoint *nbpt = new GridPoint(
						tmp,current,
						t, bending,
						f_pen, e_pen,
						current->stalling,
						MHT(tmp,T));
				p.push(nbpt);
#ifdef DEBUG
				cout<<"\tPoint "<<tmp<<" pushed. w="<<nbpt->weight<<", parent = "<<current->pt<<endl;
#endif
			}
		}// end of enqueue neighbours
	}// end of propagate

	if( success == false ){// failed to find path
		fprintf(stderr,"Error: failed to find path\n");
		exit(1);
	}
#ifdef DEBUG
	else{ printf("Success - start to backtrack\n"); }
#endif
	//////////////////////////////////////////////////////////////////////////////////

	// backtrack phase
	while( current != NULL ){
		cout<<"t="<<current->time<<", pos="<<current->pt<<endl;
		current = current->parent;
	}

	p.free();
}

// compare which net should be routed first
int Router::cmpNet(const void * id1, const void * id2){
	int i1 = *(int*)id1;
	int i2 = *(int*)id2;
	//Net * n1 = &chip.prob[idx].net[i1];
	//Net * n2 = &chip.prob[idx].net[i2];
	Net * n1 = &pProb->net[i1];
	Net * n2 = &pProb->net[i2];
	Block b1,b2;
	// get two bounding box
	b1 = getBoundingBox(n1->pin[0],n1->pin[1]);
	b2 = getBoundingBox(n2->pin[0],n2->pin[1]);
	// pin1's source inside bounding box 2,should route 1 first
	if( ptInRect(b2,n1->pin[0].pt) ) return -1;
	// pin2's source inside bounding box 1,should route 2 first
	else if( ptInRect(b1,n2->pin[0].pt) ) return 1;

	// use manhattance to judge
	int m1 = MHT(n1->pin[0].pt,n1->pin[1].pt);
	int m2 = MHT(n2->pin[0].pt,n2->pin[1].pt);
	return m1-m2;
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

int wrapper(const void *id1,const void *id2){
	//very nasty trick here... wrap the cmpNet in order to
	//match the signature of qsort's last parameter
	static Router tmp;
	return tmp.cmpNet(id1,id2);
}

void Router::sort_net(Subproblem *pProb, int * netorder){
	// do nothing here now, just initialize
	int i;
	int N=pProb->nNet;
	for(i=0;i<N;i++) netorder[i]=i;
	qsort(netorder,N,sizeof(int), wrapper);
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
int Router::fluidic_check(int which, const Point & pt,int t){
	int i;
	// for each routed net, 
	// check if the current routing net(which) violate fluidic rule
	// we have known the previous routed net 
	// from netorder[0] to netorder[i]!=which
	// t's range: [1..T]
	for(i=0;i<netcount && netorder[i] != which;i++){
		int checking = netorder[i];
		// static fluidic check
		/*
		if( !(abs(pt.x - path[checking][t].x) >=2 ||
		      abs(pt.y - path[checking][t].y) >=2) )
			return i;
		// dynamic fluidic check
		if ( !(abs(pt.x - path[checking][t-1].x) >=2 ||
		       abs(pt.y - path[checking][t-1].y) >=2) )
			return i;
			*/
	}
	return 0;
}

