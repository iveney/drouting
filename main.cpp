// Filename : main.cpp
// the main routing algorithm
//
// Author : Xiao Zigang
// Modifed: < Tue Mar 17 10:39:54 HKT 2009 >
// ----------------------------------------------------------------//

#include <deque>
#include <iomanip>
#include <vector>
#include <queue>
#include "header.h"
#include "route.h"
#include "GridPoint.h"
#include "main.h"
using namespace std;

typedef heap<GridPoint*,vector<GridPoint*>,GridPoint::GPpointerCmp> GP_HEAP;

int main(int argc, char * argv[]){
	idx=1;
	// read configuration file and parse it
	read_file(argc,argv,&chip);

	// solve subproblem `idx'
	Subproblem * pProb = &chip.prob[idx];
#ifdef DEBUG
	printf("Start to solve subproblem %d\n",idx);
#endif
	
	// sort : decide net order
	netcount = pProb->nNet;
	sortNet(pProb,netorder);
#ifdef DEBUG
	printf("net order: [ ");
	for(int i=0;i<pProb->nNet;i++){printf("%d ",netorder[i]);}
	printf("]\n");
#endif

	// generate blockage bitmap
	initBlock(pProb);

	int i,j;
	N=chip.N;
       	M=chip.M;
	memset(grid,0,sizeof(grid));
	GridPoint *current;
	// start to route each net according to sorted order
	for(i=0;i<(pProb->nNet);i++){
		int which = netorder[i];
		Net * pNet = &pProb->net[which]; // according to netorder
#ifdef DEBUF
		printf("** Routing net[%d] **\n",which);
#endif
		// do Lee's propagation,handles 2-pin net only currently
		int numPin = pNet->numPin;
		//if( numPin == 3 ) {} // handle three pin net
		Point S = pNet->pin[0].pt; // source
		Point T = pNet->pin[1].pt; // sink
#ifdef DEBUG
		for(int i=0;i<numPin;i++)// output Net infor
			cout<<"\t"<<"pin "<<i<<":"<<pNet->pin[i].pt<<endl;
#endif

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
			cout<<"\tStalling Point "<<same->pt<<" pushed. w="<<same->weight<<
				", parent = "<<same->parent->pt<<endl;

			// get its neighbours( PROBLEM: can it be back? )
			vector<Point> nbr = getNbr(current->pt);
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
					if(current->parent != NULL && (*iter) == current->parent->pt) continue;
					// calculate its weight
					Point tmp(x,y);
					int f_pen=0,e_pen=0,bending=current->bend;
					int fluidic_result = fluidicCheck( which,tmp,t );
					bool electro_result = electrodeCheck( tmp );

					// fluidic constraint
					if( fluidic_result != 0 )
						f_pen = FLUID_PENALTY;

					// electro constraint
					if( !electro_result )
						e_pen = ELECT_PENALTY;

					// check bending
					if( current->parent != NULL &&
					    checkBending(tmp,current->parent->pt) == true )
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
	}// end of for each net
	return 0;
}
