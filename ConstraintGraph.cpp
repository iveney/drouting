#include <cassert>
#include "util.h"
#include "ConstraintGraph.h"

// get the real index of the GNode in ConstraintGraph::node
#define get_node_idx(n) (((n).type==ROW?0:row) + (n).idx)

//
#define access_node(n) node_list[get_node_idx(n)]

// access the edge of two nodes u,v NOTE: no self edge!
#define access_edge(u,v) edge[get_node_idx(u)][get_node_idx(v)]

#define get_color(n) node_list[get_node_idx(n)].color
#define set_color(n,color) get_color(n) = (color)
#define next_color(color) (color)==HI?LO:HI

ConstraintGraph::ConstraintGraph(int width,int height):
	row(height),col(width), 
	node_list( GNodeVector(row+col) ), 
	edge( vector< GEdgeVector >(row+col) ){
	// initialize the node's type
	for (int i = 0; i<row; i++) {
		node_list[i]=GNode(ROW,i);
	}
	for (int j = 0; j<col; j++) {
		node_list[j+row]=GNode(COL,j);
	}
	// initialize the 2nd dimension of edge
	for (size_t i= 0; i < edge.size(); i++) {
		edge[i] = GEdgeVector(row+col);
		// initialize the matrix element
		for (int j = 0; j < row+col; j++) {
			edge[i][j].u=node_list[i];
			edge[i][j].v=node_list[j];
		}
	}
}

ConstraintGraph::~ConstraintGraph(){ }

// given two node u,v check if there is e=(u,v)
bool ConstraintGraph::has_edge(const GNode &u,const GNode &v){
	GEdge &e = access_edge(u,v);
	return e.type != NOEDGE;
}

// add an edge between u,v
bool ConstraintGraph::do_add_edge(const GNode &u,const GNode &v,EType type){
	GEdge &uv = access_edge(u,v);
	GEdge &vu = access_edge(v,u);
	GNode &unode = access_node(u);
	GNode &vnode = access_node(v);
	if( uv.type == NOEDGE || uv.type == type ){// safely add edge
		uv.type = type;
		uv.count++;
		unode.ecount++;

		vu.type = type; // symmetric
		vu.count++;
		vnode.ecount++;
	}
	else
		report_exit("Trying to add incompatible edge type");
	return true;
}

// remove (one) edge between node u,v
// note that when the node becomes standalone, also rip its color
bool ConstraintGraph::remove_edge(const GNode &u,const GNode &v){
	GEdge &uv = access_edge(u,v);
	GEdge &vu = access_edge(v,u);
	GNode &unode = access_node(u);
	GNode &vnode = access_node(v);
	assert( uv.count == vu.count );

	// edge not exist, 
	if( uv.count == 0 ) return false; 

	uv.count--;
	if( uv.count == 0 ) uv.type = NOEDGE;

	unode.ecount--;
	if( unode.ecount == 0 ) erase_color(u);

	vu.count--; // symmetric
	if( vu.count == 0 ) vu.type = NOEDGE;

	vnode.ecount--;
	if( vnode.ecount == 0 ) erase_color(v);

	return true;
}

// reset the color of node to G
// return its PREVIOUS color
COLOR ConstraintGraph::erase_color(const GNode & node){
	COLOR pre;
	GNode & n = access_node(node);
	pre=n.color;
	n.color = G;
	return pre;
}

// try to add edge with color awareness
// return EXIST if the edge exists already
bool ConstraintGraph::add_edge_color(const GNode &u,const GNode &v, EType type){
	// first check if edge exist
	int a=get_node_idx(u);
	int b=get_node_idx(v);
	//GEdge & uv = access_edge(u,v);
	GEdge & uv = edge[a][b];
	if( uv.type != NOEDGE ){// already has an edge
		if( uv.type != type ) return false; // type incompatible
		else do_add_edge(u,v,type);  // safely add dummy edge
	}

	// now we ensure u-v edge NOT exist(NOEDGE)
	// check if adding this edge will conflict
	// explore all 9 cases: {G,H,V} X {G,H,V}
	assert( u != v );
	GNode & unode = access_node(u);
	GNode & vnode = access_node(v);
	COLOR ucolor = get_color(u);
	COLOR vcolor = get_color(v);
	
	// node u standalone, which means color = G
	if( ucolor == G ){
		assert( unode.ecount == 0 );
		do_add_edge(u,v,type);  // safely add edge
		if( vcolor == G ){ // G,G: color them
			set_color(unode,HI);
			if( type == DIFF ) set_color(vnode,LO);
			else set_color(vnode,HI);
		}
		else if( vcolor == HI ){// G,H
			if( type == DIFF ) set_color(unode,LO);
			else set_color(unode,HI);
		}
		else{// G,L
			if( type == DIFF ) set_color(unode,HI);
			else set_color(unode,LO);
		}
		return SUCCESS;
	}
	// v standlone(symmetric to previous case)
	else if( vcolor == G ) {
		assert( vnode.ecount == 0 );
		do_add_edge(v,u,type); // safely add edge
		if( ucolor == G ){ // G,G: color them
			set_color(unode,LO);
			if( type == DIFF ) set_color(vnode,HI);
			else set_color(vnode,LO);
		}
		else if( ucolor == HI ){// H,G
			if( type == DIFF ) set_color(vnode,LO);
			else set_color(vnode,HI);
		}
		else{// L,G
			if( type == DIFF ) set_color(vnode,HI);
			else set_color(vnode,LO);
		}
		return SUCCESS;
	}
	else{// u,v not standalone => u,v has colors, but not connected
		/*
		if( ucolor != vcolor ){// H-L or L-H
			if( type == SAME ) // type incompatible!
				return false;
			do_add_edge(u,v,type); // safely link them
			return SUCCESS;
		}
		*/
		// H-L, H-H or L-L
		// try to swap the color 
		// of one connected component
		ConstraintGraph bak(*this); // make backup
		do_add_edge(u,v,type);      // link them first
		reverse_color(v);           // reverse the coloring of v component
		COLOR u_newcolor = get_color(u);
		if( ucolor != u_newcolor ){ // if u's color changed=>2-color failed
			*this = bak;
			return FAIL;
		}
		else
			return SUCCESS;
	}
}

// recursively reverse the color of a componenet, which contains `node'
void ConstraintGraph::recur_reverse_color(const GNode &node,
		BoolVector & mark){
	int idx = get_node_idx(node);
	// check if this node has been visited
	if( mark[idx] == true ) return;
	else mark[idx] = true;

	COLOR origin_color = get_color(node);
	COLOR new_color = next_color(origin_color);
	set_color(node,new_color);

	// reverse color all its adjacent nodes
	vector<GEdge> & adj_nodes = edge[idx];
	for (int v_idx=0; v_idx<row+col;v_idx++) {
		// 1.no need to check itself
		// 2.only check if edge exist
		GEdge & e = adj_nodes[v_idx];
		if( v_idx != idx && e.type != NOEDGE ){
			assert( e.v != node );
			recur_reverse_color(e.v,mark);
		}
	}
}

// reverse the color of a component, which containing `node'
void ConstraintGraph::reverse_color(const GNode &node){
	BoolVector mark(row+col,false);
	recur_reverse_color(node,mark);
}
// color `node' using the color `to_assign' 
// and try color all its adjacent nodes
bool ConstraintGraph::recur_color(const GNode &node,COLOR to_assign){
	assert( to_assign != G ); // color will not be G!!
	COLOR cur_color = get_color(node);

	// this node has been colored to H or L
	if( cur_color == to_assign ) 
		return true; 
	// this node has colored to different color, fail!
	else if( cur_color != G ) 
		return false;

	// it is G, color it, and all its neighbours
	set_color(node,to_assign);

	COLOR opposite_color = next_color(to_assign);
	int idx = get_node_idx(node);
	vector<GEdge> & elist = edge[idx];
	for(int i = 0; i < row+col; i++) {
		GEdge & e = elist[i];
		if( i!=idx && e.count > 0){
			bool result;
			if( e.type == DIFF )
				result = recur_color(e.v,opposite_color);
			else
				result = recur_color(e.v,to_assign);
			if( result == false ) return false;
		}
	}
	
	// safely colored all neighbour
	return true;
}

// color all the connected component, but not standalone node
// try to assign color to the graph on the base of current coloring
// return true if successful
bool ConstraintGraph::try_coloring(){
	for (int i = 0; i < row+col; i++) {
		// degree must greater than 0, otherwise keep color=G
		if( node_list[i].ecount == 0 ) continue;
		// don't need to color again if has color
		if( node_list[i].color != G ) continue;
		//vector<GEdge> &elist = edge[i];
		bool result = recur_color(node_list[i],HI);
		if( result == false ) return false;
	}
	// safely colored all connected component
	return true;
}

// COL wins ROW
// if same type, smaller index wins
bool operator <(GNode u,GNode v){
	return (u.type == COL && v.type == ROW) ||
	       (u.type == v.type && u.idx < v.idx);
}

bool operator == (GNode u,GNode v){
	return u.type == v.type && u.idx == v.idx;
}

bool operator != (GNode u, GNode v){
	return !(u == v);
}
