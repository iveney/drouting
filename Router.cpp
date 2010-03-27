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
using std::random_shuffle;

const int dx[]={-1,1,0,0,0};
const int dy[]={0,0,1,-1,0};
extern const char *color_string[];


#ifdef DEBUG
int STALLING_COUNT = 0;
int MHT_DIFF = 0;
#endif

// parameter to control the searching
int MAXCFLT=10000;
int MAX_SINGLE_CFLT=1000;

const int MAX_TRYTIME=5;
int tried_time=0;

// static memeber initialization
Subproblem * Router::pProb=NULL;

// constructor of Router
// initialize the read flag, netorder, and the max_t
Router::Router():read(false),max_t(-1){
	for (int i = 0; i < MAXTIME; i++) {
		graph[i] = NULL;
	}
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
	// now only allows to solve a given subproblem
	if(argc<2) report_exit("Usage: CrossRouter filename [subproblem id]");

	//const char * filename = argv[1];
	filename = string(argv[1]);

	if( argc == 3 ) tosolve = atoi(argv[2]);
	else            tosolve = -1;   // not given in cmdline, solve all
	if( (f = fopen(filename.c_str(),"r")) == NULL ){
		char buf[MAXBUF];
		sprintf(buf,"Open file '%s' error",filename.c_str());
		report_exit(buf);
	}
	read=true;
	parse(f,&chip); // now `chip' stores subproblems
	fclose(f);
}

// allocate space for each time step of the graph
void Router::allocate_graph(){
	// index range: 1,2,...T, (totally T graphs)
	for (int i = 1; i <= T ; i++)
		graph[i] = new ConstraintGraph(W,H);
}

// free the space allocated to constraint graph
void Router::destroy_graph(){
	for (int i = 1; i <= T; i++){
		delete graph[i];
		graph[i] = NULL;
	}
}

// put initialization stuff here
void Router::init(){
	for(int i=0;i<MAXNET;i++) netorder[i]=i;
	W=chip.W;
	H=chip.H;
	T=chip.T;
	max_t = -1;
	last_ripper_id = -1;
	GridPoint::counter=0;
	memset(visited,0,sizeof(visited));
	memset(cell_used,0,sizeof(cell_used));
	// count the subnets
	subnet_count = 0;
	for(int i=0;i<pProb->nNet;i++){
		Net * pNet = &pProb->net[i];
		if( pNet->numPin == 3 )
			subnet_count+=2;
		else
			++subnet_count;
	}
	
	destroy_graph();
	allocate_graph();
}

// solve subproblem `prob_idx'
RouteResult Router::solve_subproblem(int prob_idx){
	if( read == false ) report_exit("Must read input first!");
	if( prob_idx > this->chip.nSubProblem ) 
		report_exit("Subproblem id out of bound!");
	cout<<"--- Solving subproblem ["<<prob_idx<<"] ---"<<endl;

	pProb = &chip.prob[prob_idx];
	netcount = pProb->nNet;
	init();
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
		if( success == false && tried_time++ < MAX_TRYTIME ){
			report_exit("Error: route failed!");
			// other action may be taken here, e.g., random shuffle
			// net order
			init();
			nets.clear();
			nets.insert(nets.begin(),netorder,netorder+netcount);
			random_shuffle(nets.begin(),nets.end());
		}
		result.path[which].routed=true;
		routed_count++;
	}

	// finally, output result
	cout<<"*** Subproblem ["<<prob_idx<<"] solved! ***"<<endl;
	output_result(result);

#ifdef DEBUG
	cerr<<"STALL STEP = "<<STALLING_COUNT<<endl;
	cerr<<"MHT DIFF = "<<MHT_DIFF<<endl;

	// output result to TeX file
	char buf[MAXBUF];
	sprintf(buf,"%s_%d_sol.tex",filename.c_str(),prob_idx);
	draw_voltage(result,chip,buf);
	cout<<"max time = "<<get_maxt()<<" "<<endl;
#endif

	return result ;
}

// output the result to standard output
void Router::output_result(RouteResult & result){
	int i,j,count=0;
	char used_cell[MAXGRID][MAXGRID];
	memset(used_cell,0,sizeof(used_cell));
	// for each net
	for (i = 0; i < netcount; i++) {
		const NetRoute & net_path = result.path[i];
		// for each subnet in a net
		for (j = 0; j < net_path.num_pin-1; j++) {
#ifdef DEBUG
			Point src = get_pinpt(i,j);
			Point dst = get_netdst_pt(i);
			Pin src_pin,dst_pin;
			src_pin.pt = src; dst_pin.pt = dst;
			//int mht_dist = MHT(src,dst);
			Block bb = get_bbox(src_pin,dst_pin);
			int diff = 0;
			map<Point,bool> detour;
#endif
			cout<<"net["<<i<<"]:"<<this->T<<endl;
			const PtVector & route = net_path.pin_route[j];
			// output each time step
			for(size_t k=0;k<route.size();k++){
				cout<<"\t"<<k<<":"
				<<route[k]<<endl;
				int x = route[k].x, y = route[k].y;
#ifdef DEBUG
				// count the stalling 
				if( k>0 && route[k] != dst && 
				    route[k] == route[k-1] ){
					cerr<<"STALLING here"<<endl;
					STALLING_COUNT++;
				}
				// count the MHT difference of this subnet
				// check if this is a detour step
				// NOTE: for this subnet, same cell only count once
				if( !pt_in_rect(bb,route[k]) && 
				    !detour[route[k]]) {
					diff++;
					cerr<<"detour:"<<route[k]<<endl;
				}
				// END of counting
#endif
				if( used_cell[x][y] == 0 ){
					count++;
					used_cell[x][y]=1;
				}	
			}
#ifdef DEBUG
			MHT_DIFF += diff;
			cerr<<"net("<<i<<","<<j<<") DIFF = "<<diff<<endl;
#endif
		}
		// trick: do not count the pin into cell used
		count-=pProb->net[i].numPin; 
	}
	output_voltage(result);
#ifdef DEBUG
	cout<<"**** total cell used : "<<count<<" *****"<<endl;
#endif
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
	
		// output ROW electrode assignment
		for (j = 0;j<H; j++) {
			COLOR clr = p_graph->node_list[j].color;
			result.v_row[i-1][j] = clr;
			if(clr==G) continue;
			cout<<"("<<j<<"="<<color_string[clr]<<") ";
			// get the activated cell
			for(int k=0;k<W;k++){
				COLOR c_clr = p_graph->node_list[k+H].color;
				if(clr==HI && c_clr==LO||
				   clr==LO && c_clr==HI) {
					Point tmp(k,j);
					activated.push_back(tmp);
				}
			}
		}

		// output COL electrode assignment
		cout<<endl<<"COL:\t";
		for (j = 0; j < W; j++) {
			COLOR clr = p_graph->node_list[j+H].color;
			result.v_col[i-1][j] = clr;
			if(clr==G) continue;
			cout<<"("<<j<<"="<<color_string[clr]<<") ";
		}
		cout<<endl;
		cout<<"Act:\t";

		// output activated cell
		for(size_t k=0;k<activated.size();k++){
			cout<<activated[k]<<" ";
			result.activated[i-1].push_back(activated[k]);
		}
		cout<<endl;
	}
	// sets other elctrode to G
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
void Router::solve_all(){
	// note that subproblem starts from 1
	for(int i=1;i<=chip.nSubProblem;i++)
		route_result.push_back(solve_subproblem(i));
	//return route_result;
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
	memset(visited,0,sizeof(visited));
	MAXCFLT=10000;
	MAX_SINGLE_CFLT=1000;

	// do Lee's propagation,handles 2-pin net only currently
	GridPoint *current;

	// initialize the heap
	GP_HEAP p;

	// start time = 0, source point = src, no parent
	GridPoint::counter = 0;
	GridPoint *gp_src = new GridPoint(src,NULL); 
	gp_src->distance = MHT(src,dst);
	p.push(gp_src); // put the source point into heap

	int t=0;               // current time step
	bool success = false;  // mark if this net is routed successfully
	FLUIDIC_RESULT fluid_result;
	while( !p.empty() ){
		if( p.size() > MAXHEAPSIZE ){
			// jump out to rip up others
			cerr<<"heap size exceed"<<endl;
			break;
		}
		// if there are too much conflict, ripup and reroute
		if( conflict_net.total > MAXCFLT ){
			cerr<<"Too much conflict"<<endl;
			break;
		}
		// if there is some net causing too much conflict
		int mid = conflict_net.max_id;
		if( mid>=0 && 
			conflict_net.conflict_count[mid] > MAX_SINGLE_CFLT ){
			cerr<<"Too much conflict by net ["<<mid<<"]"<<endl;
			break;
		}
		// get wave_front and propagate its neighbour
		//p.sort();
#ifdef PRINT_HEAP
		cout<<"------------------------------------------------"<<endl;
		cout<<"[before pop]"<<endl;
		output_heap(p);
#endif 
		current = p.top();
		p.pop();
#ifdef PRINT_HEAP
		cout<<"[after pop]"<<endl;
		cout<<"Pop "         <<current->pt
		    <<" at time "    <<current->time
		    <<", queue size="<<p.size()<<endl;
		output_heap(p);
#endif 

		// sink reached, but need to check whether it stays
		// there will block others
		if( current->pt == dst ) {
			int reach_t = current->time;
			bool fail = false;
			// see whether it can stay in destination
			// do fluidic/electrode check
			if( dst != chip.WAT ){
				// IMPORTANT: for WAT, no need to check!
				for (int i = reach_t+1; i <= T; i++) {
					fluid_result = fluidic_check(which,
							pin_idx, current->pt,
							i,result,conflict_net);
					if( fluid_result == VIOLATE ){
						//cout<<"fluidic t="
						//   <<current->time<<endl;
						fail = true;
						break;
					}

					bool not_elect_violate=electrode_check(
							which,pin_idx,
							current->pt,
							current->pt,i,
							result,conflict_net,0);
					if( !not_elect_violate ){
					//cout<<"electric t="
					//<<current->time<<endl;
						fail = true;
						break;
					}
				}// end of for
			} // end of if

			// safely enter destination and stay
			if( !fail ){
				cout<<"Find "<<dst
				    <<" at time "<<current->time<<"!"<<endl;
				if( reach_t > max_t ) max_t = reach_t;
				success = true;
				break;
			}
			// else, could not stay at sink point now
		}//end of if(current_pt=dst)

		// sink not found, continue to search here...
		
		// do pruning here:
		// 1. MHT>time left(impossbile to reach dest)
		// 2. time exceed
		t = current->time+1;
		int time_left = this->T - current->time;
		int remain_dist = MHT(current->pt,dst);
		if( (remain_dist > time_left) || (t > this->T) ){
			if( p.size() != 0 ) continue;
			else{// fail,try rip-up and re-route
				cerr<<"Give up: time left="<<time_left
				    <<", remaining MHT="<<remain_dist<<endl;
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
		if( pNet->numPin == 2 )
			success=route_2pin(which,result,conflict_net);
		else
			success=route_3pin(which,result,conflict_net);

		if( success == false ){
			// first net can not be route: routing failed
			// NOTE: change strategy: allow detour now
			if(netorder[0]==which) return false;

			cerr<<"** Warning: route net["<<which
			    <<"] failed, try ripup-reroute **"<<endl;
			bool ret;
			ret = ripup_reroute(which,result,conflict_net);

			// cannot do ripup-reroute
			if( ret == false ) return false;

			// output the new net order
			output_netorder(netorder,netcount);
			//cout<<"after rip"<<endl;
			//output_result(result);
			cout<<"re-route net ["<<which<<"]"<<endl;
		}
	}while(success == false); // keeps routing it...

	// routing succeed
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
		// route the net with shorter MHT first
		swap(a,b);
	}

	cout<<"subnet routing order: <"<<a<<" "<<b<<">"<<endl;
	result.path[which].clear();
	
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

// choose a net to be ripped by drawing a lot
// NOTE: 1. should choose a net that has been routed
//       2. should choose a net which is not the current routing net
//       3. in order to prevent deadlock, do not choose last_ripper
// PROB: sometimes there is only one net causing conflict, and that is last_ripper
//       should choose another routed net 
int Router::choose_ripped(int which, RouteResult & result,
		const ConflictSet & conflict_net){
	// first calculate the probabilities
	int chance[MAXNET];
	memset(chance,0,sizeof(chance));
	vector<int> routed;

	// compute each 'accumulative' ripup probabilites of each net
	// NOTE: sometimes, the unroutable net is not caused by other nets(really?)
	// then the chances of all nets and sum will be 0
	// Give a base probability of 1 to each net except itself
	// Don't give chance to the last_ripper
	for(int i=0;i<conflict_net.net_num;i++){
		// don't need to compute the net being routed
		int base=0;
		if(i!=0) base=chance[i-1];
		chance[i]=base;
		if( i!=which && i!=last_ripper_id )
			chance[i]+=1+conflict_net.conflict_count[i];
		//printf("chance[%d]=%d\n",i,chance[i]);
		
		// put all routed net id (except the last ripper) into a set
		if( result.path[i].routed == true && i!=last_ripper_id)
			routed.push_back(i);
	}

	int sum=chance[conflict_net.net_num-1];
	if(sum==0) // this only happens when `which' is the first net
		return -1;
	const int drawlot = rand()%sum;
	//printf("lot = %d, sum = %d\n",drawlot,sum);
	int to_rip_id=0, counter=0;
	while(1){// see which slot the lottery is in
		if( drawlot >= chance[to_rip_id] )
			to_rip_id++;
		else
			break;
		if( counter++ > MAX_COUNTER ) 
			report_exit("Infinite loop 1");
	}
	counter=0;
	
	// don't rip itself, and don't rip the one that was the last ripper
	// if unfortunately the to_rip_id is in either case, just randomly pick 
	// one that was routed 
	if( to_rip_id == which || to_rip_id == last_ripper_id ||
		result.path[to_rip_id].routed ==false ){
		//printf("last_rip=%d, choose from: ", last_ripper_id);
		//for(unsigned int i=0;i<routed.size();i++)
			//printf("%d ",routed[i]);
		//printf("\n");
		if(routed.size() == 0 ) return -1; // failed to choose
		to_rip_id=routed[rand()%routed.size()];
	}
	
	return to_rip_id;
}

// for a given net `which', rip up the net causing most conflict
// also re-order the netorder
// POSTCONDITION: the net order will be changed
bool Router::ripup_reroute(int which,RouteResult & result,
		ConflictSet &conflict_net){
	// --- cancel the route result of some conflict net
	// --- now use the most conflict net=`rip_netid'
	// int rip_netid = conflict_net.rip_netid;
	// assert(rip_netid>=0);

	// what about for 3-pin net that is causing constraint on itself?

	int rip_netid;

	// draw a lot to decide which net to rip
	rip_netid = choose_ripped(which,result,conflict_net);
#ifdef DEBUG
	printf("DEBUG: which = %d, rip_netid = %d, last = %d\n",
			which,rip_netid,last_ripper_id);
#endif

	// returns -1 means cannot find a net to rip
	if( rip_netid < 0 ) return false;

	cout<<"** ripup net ["<<rip_netid<<"]";
#ifdef DEBUG
	cout<<" conflict count="<<conflict_net.conflict_count[rip_netid]
	    <<" last ripper = "<<last_ripper_id;
#endif
	cout<<" **"<<endl;

	// remember to clear the cell used counting of it
	for (int i=0;i<result.path[rip_netid].num_pin-1;i++) {
		set<Point>::iterator it;
		set<Point> & use = result.path[rip_netid].cellset[i];
		for (it=use.begin();it!=use.end();++it) {
			Point pt=(*it);
			--cell_used[pt.x][pt.y];
		}
	}

	// need to remove the cell used by it also
	result.path[rip_netid].clear(); // cancel the routed path

	// clear the conflict result of this net
	conflict_net =  ConflictSet(conflict_net.net_num);

	// find there index in netorder
	int rip_netid_inorder=-1,which_id_inorder=-1;
	for(int i = 0; i < netcount; i++) {
		if( netorder[i] == rip_netid) rip_netid_inorder = i;
		if( netorder[i] == which) which_id_inorder = i;
	}

	// re-push into queue(so that re route `rip_netid')
	nets.push_front(rip_netid);

	// re-order route order
	IntVector temp(netorder,netorder+netcount);
	temp.erase(temp.begin()+rip_netid_inorder);          //remove net=max
	temp.insert(temp.begin()+which_id_inorder,rip_netid);//insert after which
	copy(temp.begin(),temp.end(),netorder);

	// remove the edges caused by this net in the graph
	// now just clear all graph, and re-add constraint
	destroy_graph();
	allocate_graph();
	for (int i=0;i<netcount && netorder[i]!=which; i++) {
		int checking_idx = netorder[i];
		//if( i == rip_netid_inorder ) continue; // do not add this net
		const NetRoute & route = result.path[checking_idx];
		for(int j=0;j<route.num_pin-1;j++){
			const PtVector & pin_path = 
				result.path[checking_idx].pin_route[j];
			// reconstruct the graph
			update_graph(checking_idx,j,pin_path,result);
		}
	}

	// `which' becomes the last ripper
	last_ripper_id = which; 

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
	// get its neighbours
	PtVector nbr = get_neighbour(from_pt);
	const int t = (gp_from->time + 1);

	// enqueue neighbours
	//GridPoint * parent_of_from = gp_from->parent; 

	for(size_t i=0;i<nbr.size();i++){
		int x=nbr[i].x,y=nbr[i].y;
		Point moving_to(x,y);
		// 1.check if there is blockage 
		// 2.check if ((x,y),t) has been visited

		if( blockage[x][y] == BLOCK ) continue;
		DIRECTION dir = pt_relative_pos(moving_to,from_pt);
		if( visited[x][y][t] == 1 && dir != STAY ) continue; 
		visited[x][y][t] = 1;

		//int bending=0;//bending=gp_from->bend;

		// fluidic constraint check
		FLUIDIC_RESULT fluid_result=fluidic_check(which,pin_idx,
				moving_to,t,result,possible_nets);

		// TEST: currently this will not happen
		if( fluid_result == SAMENET ){
			// multipin net
			// report_exit("fluid_result == SAMENET");
		} 
		else if( fluid_result == VIOLATE ){
			//assert(conflict_netid != which);
			//possible_nets.increment(conflict_netid);
			//cout<<"net "<<which<<" fluidic violation:"
			//<<from_pt<<"->"<<moving_to<<" time="<<t<<endl;
			continue;
		}

		// electro constraint check
		bool not_elect_violate=electrode_check(which,pin_idx,
			       	moving_to,from_pt,t,result,possible_nets,0);
		if( !not_elect_violate ){
			//cout<<"net "<<which<<" electrode violation:"
			//<<from_pt<<"->"<<moving_to<<endl;
			continue;
		}

		// length update, now ignore it here
		int newlen = gp_from->length;
		if( dir != STAY ) ++newlen;
#ifdef NOLENGTH
		newlen=0; // force to 0
#endif

		// cell used update
		double cell;
		if (cell_used[x][y]>0 || dir==STAY)
			cell=0.0;
		else
			cell=1.0;
		cell*=CELL_FACTOR;

		/*double cell = CELL_FACTOR*(subnet_count - cell_used[x][y]);*/
		assert(cell>=0); 

		// finally push this into heap
		GridPoint *nbpt = new GridPoint(
				moving_to,gp_from,
				//t,newlen,cell,0,0,
				//now for `cell' use accumulate method
				t,newlen,cell+gp_from->cell,0,0,
				MHT(moving_to,dst));
		//cout<<" new weight="<<nbpt->weight<<endl;
		p.push(nbpt);
		has_pushed = true; // marks at least one status is pushed
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
	
	set<Point> & used = result.path[which].cellset[pin_idx];
	// trace back from dest to src
	while( current != NULL ){
		pin_path.insert(pin_path.begin(),current->pt);
		if( used.find(current->pt)==used.end() ){
			used.insert(current->pt);
			++cell_used[current->pt.x][current->pt.y];
		}
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
// simply re-construct the constraint graph
void Router::update_graph(int which,int pin_idx,
		const PtVector & pin_path,
		RouteResult & result){
	// we need to update all the coloring status 
	// for every time step until it reaches the dest, 
	// however, do not add the stalling frame!(why)
	ConflictSet dummy(netcount);
	for (size_t i = 1; i < pin_path.size(); i++) {
		// i is time
		Point p = pin_path[i],q=pin_path[i-1];
		//DIRECTION dir = pt_relative_pos(q,p);
		Point dst = get_netdst_pt(which);
		bool success = electrode_check(which,pin_idx,
				p,q,i,result,dummy,1);
		// IMPORTANT: return value must be true here!
		assert(success == true);
	}
}

// print out what is in the heap
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
// POSTCONDITION: smaller value goes first
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
	return m1-m2; // original
	//return m2-m1;
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
// `d1' is moving to a point `pt' at time `t' from `parent_pt`
// determine whether it will violate electrode constraint
// 1  2  3  4
// 5  6->7  8
// 9  10 11 12
// suppose move 6->7, then row 5, col 3 (C3,R5) activated(type 1 constraint)
// ensure it will not be affected by others if (C3,R5) activated(type 2)
// ensure 2 nets will not cause conflict on another net:1,2,4,9,10,12(type 3)
bool Router::electrode_check(int which, int pin_idx,
		const Point & pt,const Point & parent_pt,int t,
		RouteResult & result,
		ConflictSet & conflict_net,int control){
	// TODO: for 3-pin net, we need to recover path for another net
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
		NetRoute & route = result.path[checking_idx];
		// for each subnet(at most 2)
		for (int j = 0; j<route.num_pin-1; j++) {
			// do not check it self!
			if( checking_idx == which && j == pin_idx ) continue;

			PtVector & pin = route.pin_route[j];
			// pin[t]=location of d2 at time t(activated at t-1)
			// IMOPRTANT:some droplet may disappear(waste disposal)
			if( t >= (int)pin.size() ) continue;

			// if two droplet's sharing same activating row/column
			// NO need to check the constraint!( my assumption )
			// IMPORTANT: the idea is correct 

			// special case for 3-pin net: allow violation if merge
			// NOTE: since when chekcing 3-pin net, it implies that
			// this is the last net to be checked
			// hence it means that there will be NO violation when
			// activating pt considering other nets except `which'
			// hence the modify of the 3-pin net is always true

			if(route.num_pin == 3){
				// if satisfied merge condition: another net is 
				// waiting in the sink point
				if( pin[t].x == pt.x && abs(pin[t].y-pt.y)<=1||
				    pin[t].y == pt.y && abs(pin[t].x-pt.x)<=1){
					continue;
				}
				// we can change the route for 1st one to 2nd
				// subnet later
			}

			// Type 2: check if d1+other-net affect d2
			if( dir1 != STAY ){
				add_result=check_droplet_conflict(
						parent_pt,pt,
						pin[t-1],pin[t],
						p_graph,t,control);
				if( add_result == false ){
					conflict_net.increment(checking_idx);
					return false;
				}
			}
			// Type 3: check if d2+other-net affect d1
			DIRECTION dir2 = pt_relative_pos(pin[t-1],pin[t]);
			if( dir2 != STAY ){
				add_result=check_droplet_conflict(
						pin[t-1],pin[t],
						parent_pt,pt,
						p_graph,t,control);
				if( add_result == false ){
					conflict_net.increment(checking_idx);
					return false;
				}
			}
			
		} // end of for j

		// after the checking of the same net, break the for loop
		// NOTE that this has already take 3-pin into account
		// no matter which is routed first
		if( checking_idx==which ) break; 
	} // end of for i

	// no electro constraint violation
	return true;
#undef check_result
}

// check whether droplet d1 will cause conflict on droplet d2 at time t
// where S1,T1 are d1's location
// where S2,T2 are d2's location
// PRE-CONDITION: d1 is moving!
bool Router::check_droplet_conflict(
		const Point & S1, const Point & T1,
		const Point & S2, const Point & T2,
		ConstraintGraph * p_graph,
		int t,int control){
	DIRECTION dir1 = pt_relative_pos(S1,T1);
	if( dir1 == STAY ) return true;
	DIRECTION dir2 = pt_relative_pos(S2,T2);

	bool result;
	// check if the activation of T1.y(d1's dest.y) affect d2
	// note that if T1.y has been activated, no need to check
	if( !(dir2 != STAY && T1.y == T2.y) ){
		result = try_add_edge(ROW,T1.y,S2,T2,t,p_graph);
		if( result == false ) return false;
	}

	// check if the activation of T1.x(d1's dest.x) affect d2
	if( !(dir2 != STAY && T1.x == T2.x) ){
		result = try_add_edge(COL,T1.x,S2,T2,t,p_graph);
		if( result == false ) return false;
	}

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
// IMPORTANT: seems we need to check path[t] and net[which] at t-1
#define FLUIDIC_VIOLATE(pt,t,ep) \
	(!(abs((pt).x - path[(t)-(ep)].x) >=2 || \
	   abs((pt).y - path[(t)-(ep)].y) >=2))
#define STATIC_VIOLATE(pt,t) \
	FLUIDIC_VIOLATE(pt,t,0)
#define DYNAMIC_VIOLATE(pt,t) \
	FLUIDIC_VIOLATE(pt,t,1)
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
			// path[t]=location of d2 at time t(activated at t-1)
			// IMOPRTANT:some droplet may disappear(waste disposal)
			if( t >= (int)path.size() ) continue;
			if( STATIC_VIOLATE(pt,t) || 
		            DYNAMIC_VIOLATE(pt,t) ){
				// ??? PROBLEM here
				if( pProb->net[i].pin[1].pt != this->chip.WAT )
					conflict_set.increment(checking_idx);
				return VIOLATE;
			}
		} // end of for j
	} // end of for i, now i == which or i>=netcount
	
	// check for 3-pin net
	const NetRoute & route = result.path[which];
	if( route.num_pin == 3 ){
		int another_idx = 1-pin_idx;
		if( route.reach_time[another_idx] == -1 )
			return SAFE;
		
		const PtVector & path = route.pin_route[another_idx];

		// check if merge condition satisfies(1-4)
		//     1
		//  2 d2 3
		//     4
		if( path[t].x == pt.x && abs(path[t].y-pt.y)==1|| 
		    path[t].y == pt.y && abs(path[t].x-pt.x)==1){
			// check if another net reached the point at t
			if ( route.reach_time[another_idx] >= t )
				return VIOLATE;
			return SAMENET;  // they should merge
		}
		// check if violate(5-8), but no that not like 9
		//  5  1 6
		//  2 d2 3 9
		//  7  4 8
		else if( MHT(path[t],pt) == 2 ){
			if( !(pt.x == path[t].x || pt.y ==path[t].y) ){
				//cout<<"violate here "<<pt<<endl;
				return VIOLATE;
			}
		}
	}
	return SAFE;
#undef FLUIDIC_VIOLATE
#undef STATIC_VIOLATE
#undef DYNAMIC_VIOLATE
}

// solves the command line argument
ResultVector Router::solve_cmdline(){
	// tosolve is not given in cmdline
	if( this->tosolve == -1 ) 
		solve_all();
	else
		route_result.push_back(solve_subproblem(tosolve));
	return route_result;
}

// returns the maximum time used
int Router::get_maxt() const{
	return this->max_t;
}
