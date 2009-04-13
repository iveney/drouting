#include <deque>
#include <cassert>
#include "ConstraintGraph.h"
using std::deque;

#define access_list(node) ((node).type==ROW?r_list:c_list)[(node).idx]
#define get_color(node) ((node).type==ROW? \
		r_color[(node).idx]:c_color[(node).idx])
#define next_color(color) ((color)==H?L:H)

// given a node u, check if there is an edge to node v
GNodeSet::iterator ConstraintGraph::find_edge(const GNode &u,const GNode &v){
	return access_list(u).find(v);
}

// given two node u,v check if there is e=(u,v)
bool ConstraintGraph::has_edge(const GNode &u,const GNode &v){
	set<GNode> & lu = access_list(u);
	if( find_edge(u,v) == lu.end() ) return false;
	else return true;
}

// add an edge between node u-v, regardless the color
bool ConstraintGraph::add_edge(const GNode &u,const GNode &v){
	// avoid duplicate edge
	if( has_edge(u,v) ) return false;
	set<GNode> & lu = access_list(u);
	set<GNode> & lv = access_list(v);
	lu.insert(v);
	lv.insert(u); // symmetric
	return true;
}

// try to insert edge, but color awareness
// return EXIST if the edge exists already
ADD_EDGE_RESULT ConstraintGraph::add_edge_color(const GNode &u,const GNode &v){
	if( has_edge(u,v) ) return EXIST;
	// check if adding this edge will conflict
	// explore all 9 cases
	// {G,H,V} X {G,H,V}
	return SUCCESS;
}

// reset the color of node to G
COLOR ConstraintGraph::erase_color(const GNode & node){
	COLOR pre;
	switch(node.type){
	case ROW:
		pre=r_color[node.idx];
		r_color[node.idx] = G;
		break;
	case COL:
		pre=c_color[node.idx];
		c_color[node.idx] = G;
		break;
	}
	return pre;
}

// remove the edge between node u,v
bool ConstraintGraph::remove_edge(const GNode &u,const GNode &v){
	set<GNode>::iterator upos= find_edge(u,v);
	set<GNode> & lu = access_list(u);

	// edge not exist
	if( upos == lu.end() ) return false;

	set<GNode> & lv = access_list(v);
	set<GNode>::iterator vpos= find_edge(v,u);
	erase_color(u);
	erase_color(v);
	lu.erase(upos);
	lv.erase(vpos); // symmetric

	return true;
}

// color `node' using the color `to_assign' 
// and try color all its adjacent nodes
bool ConstraintGraph::recur_color(const GNode &node,COLOR to_assign){
	assert( to_assign != G ); // color will not be G!!
	COLOR cur_color = get_color(node);

	// this node has been colored
	if( cur_color == to_assign ) 
		return true; // be H or L
	else if( cur_color != G ) 
		return false;// has colored to different color

	// it is G, color all its neighbours
	
	GNodeSet & l = access_list(node);
	COLOR adj_color = next_color(to_assign);
	GNodeSet::iterator it;
	for(it=l.begin();it!=l.end();it++){
		if( recur_color( *it, adj_color ) == false )
			return false;
	}
	// safely colored all neighbour
	return true;
}

// color all the connected component, but not standalone node
// try to assign color to the graph on the base of current coloring
// return true if successful
bool ConstraintGraph::try_coloring(){
	for (int i = 0; i < row; i++) {
		// degree must greater than 0, otherwise keep color=G
		if( r_list[i].empty() ) continue;
		// don't need to color again if has color
		if( r_color[i] != G ) continue;
		bool result = recur_color(GNode(ROW,i), H);
		if( result == false ) return false;
	}
	for (int j = 0; j < col; j++) {
		// degree must greater than 0, otherwise keep color=G
		if( c_list[j].empty() ) continue;
		// don't need to color again if has color
		if( c_color[j] != G ) continue;
		bool result = recur_color(GNode(COL,j), H);
		if( result == false ) return false;
	}
	// safely colored all connected component
	return true;
}
