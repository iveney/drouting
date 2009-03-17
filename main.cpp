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
using namespace std;

BYTE blockage[MAXGRID][MAXGRID];	// Blockage bitmap
Chip chip;				// Chip data and subproblem
Grid grid[MAXNET][MAXGRID][MAXGRID];	// record the routes, grid[i][x][y]: the time step that net i occupies (x,y)
int N,M;				// row/column count
int netorder[MAXNET];			// net routing order
int netcount;				// current subproblem's net count
Point path[MAXNET][MAXTIME];		// the routing path : path[i][t]: net i's position at time t
int idx = 1;				// problem to solve

int main(int argc, char * argv[]){
	// read configuration file and parse it
	init(argc,argv,&chip);

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
			printf("\tpin[%d]:\n",pNet->pin[i].pt);
#endif

		// p and q is two heap
		// initialize the heap
		priority_queue<GridPoint> p,q;
		GridPoint S();
		p.push(S);
		//priority_queue<GridPoint>::iterator qit;

		int t=0;
		bool success = false;
		while(!p.empty()){// propagate process
			t++;
			if( t > MAXTIME+1 ){ // timing constraint violated
				fprintf(stderr,"Exceed route time!\n");
				// try rip-up and re-route?
				// reroute();
				success = false;
				break;
			}
#ifdef DEBUG
			printf("t=%d\n",t);
#endif
			//q=p;        // copy p to q
			//p.clear();

			// get wave_front and propagate its neighbour
			//while( !p.empty() ){
			GridPoint current = p.top();
			p.pop();
#ifdef DEBUG
			cout<<"Propagating"<<current.pt<<endl;
#endif
			// find the sink, horray!
			if( current.pt == T ) {
#ifdef DEBUG
				printf("Find %d!\n",T);
#endif
				success = true;
				break;
			}

			// enqueue stalling (same position)
			GridPoint same=current;
			same.parent = current;
			same.time++;
			same.stalling++;
			same.weight+=2;    // how to control the stalling weight?
			p.push(same);

			// get its neighbours( PROBLEM: can it be back? )
			vector<Point> nbr = getNbr(current.pt);
			vector<Point>::iterator iter;

			// enqueue neighbours
			for(iter = nbr.begin();iter!=nbr.end();iter++){
				int x=(*iter).x,y=(*iter).y;
				// its parent should not be propagated again
				// also blockage should be check
				if( *iter != current.parent.pt && 
				    !blockage[x][y] ){ 
					// calculate its weight
					Point tmp(x,y);
					int f_pen=0,e_pen=0,bending=current.bend;
					int fluidic_result = fluidicCheck( which,tmp,t );
					bool electro_result = electrodeCheck( tmp );

					// fluidic constraint
					if( fluidic_result != 0 )
						f_pen = FLUDIC_PENALTY;

					// electro constraint
					if( !electro_result )
						e_pen = ELECT_PENALTY;

					// check bending
					if( checkBending() == true )
						bending++;
#ifdef DEBUG
					cout<<"\tPoint "<<tmp<<" pushed."<<endl;
#endif
					// (*qit).pt is the parent
					GridPoint gp(tmp,	//position
						     current,   //parent
						     t,
						     bending,
						     f_pen,
						     e_pen,
						     current.stalling;
						     );
					p.push_back(gp);
				}
			}// end of enqueue neighbours
			//}
		}// end of propagate
		//}while(!success); // end of propagate

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
}
return 0;
}
