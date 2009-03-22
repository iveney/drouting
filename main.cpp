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
#include "route.h"
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
		for(int i=0;i<numPin;i++)
			cout<<"\t"<<"pin "<<i<<" src:"<<pNet->pin[i].pt<<endl;
#endif

		// initialize the heap
		vector<GridPoint *> resource;
		GP_HEAP p;
		GridPoint *src = new GridPoint(S,NULL); // start time = 0, source point = S, no parent!
		src->distance = MHT(S,T);
		resource.push_back(src);
		p.push(src);

		int t=0;
		bool success = false;
		while( !p.empty() ){// propagate process
			// get wave_front and propagate its neighbour
			cout<<"------------------------------------------------------------"<<endl;

			cout<<"before pop:"<<endl;
			p.sort();
			for(int i=0;i<p.size();i++)
				cout<<i<<" "<<(p.c[i])->pt<<" t="<<p.c[i]->time<<" w="<<p.c[i]->weight<<endl;

			////
			GridPoint *current = p.top();
			p.pop();
			////

			cout<<"after pop:"<<endl;
			p.sort();
			for(int i=0;i<p.size();i++)
				cout<<i<<" "<<(p.c[i])->pt<<" t="<<p.c[i]->time<<" w="<<p.c[i]->weight<<endl;

			cout<<"Pop "<<current->pt<<" at time "<<current->time<<", queue size="<<p.size()<<endl;
			t = current->time+1;
			if( t > MAXTIME+1 ){ // timing constraint violated
				if( p.size() != 0 )
					continue;
				else{
					fprintf(stderr,"Exceed route time!\n");
					// try rip-up and re-route?
					// reroute();
					success = false;
					break;
				}
			}
#ifdef DEBUG
			printf("t=%d\n",t);
			cout<<"Propagating"<<current->pt<<endl;
#endif
			// find the sink, horray!
			if( current->pt == T ) {
#ifdef DEBUG
				cout<<"Find "<<T<<"!\n";
#endif
				success = true;
				break;
			}

			// same position, stall for 1 time step
			GridPoint *same = new GridPoint( current->pt,current,
					t,current->bend,  current->fluidic,
					current->electro, current->stalling+STALL_PENALTY,
					current->distance );
			resource.push_back(same);
			p.push(same);

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
#ifdef DEBUG
					cout<<"\tPoint "<<tmp<<" pushed.parent = "<<current->pt<<endl;
#endif
					// (*qit).pt is the parent
					GridPoint *nbpt = new GridPoint(tmp,	//position
							current,   //parent
							t,
							bending,
							f_pen,
							e_pen,
							current->stalling,
							MHT(tmp,T)
						    );
					resource.push_back(nbpt);
					p.push(nbpt);
				}
			}// end of enqueue neighbours
		}// end of propagate

		if( success == false ){// failed to find path
			fprintf(stderr,"Error: failed to find path\n");
			exit(1);
		}
#ifdef DEBUG
		else{
			printf("Success - start to backtrack\n");
		}

		// output the maze
		/*
		   for(int y=MAXGRID-1;y>=0;y--){
		   for(int x=0;x<MAXGRID;x++){
		   int tmp=grid[which][x][y];
		   if(tmp == INF)
		   cout<<setw(4)<<"#";
		   else
		   cout<<setw(4)<<tmp;
		   }
		   cout<<endl;
		   }
		   */
#endif
		// backtrack phase for the net `which'
		int arrive_time = grid[which][T.x][T.y];
		Point back,new_back=T;
		cout<<"net["<<which<<"]:"<<arrive_time<<endl<<setw(8)<<arrive_time<<" : "<<new_back<<endl;
		DIRECTION dir = STAY;
		for(j=arrive_time;j<=chip.time;j++) path[which][j] = T;
		for(j=arrive_time;j>=1;j--){
			// try not to change direction, but need to decide first dir
			dir = PtRelativePos(new_back,back); // decide the previous DIR
			back=new_back;
			new_back = traceback_line(which,j,back,dir);
			cout<<setw(8)<<j-1<<" : "<<new_back<<endl;
			// if impossible, report error
			path[which][j-1] = new_back;
		}

		for(size_t i=0;i<resource.size();i++)
			delete resource[i];
	}
	return 0;
}
