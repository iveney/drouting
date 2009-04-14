#include <cassert>
#include "ConstraintGraph.h"

#define access_list(node) ((node).type==ROW?r_list:c_list)[(node).idx]
#define get_color(node) ((node).type==ROW? \
		r_color[(node).idx]:c_color[(node).idx])
#define set_color(node,color) get_color(node) = (color)
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

// add and edge between u,v
// this method should not be used directly!
void ConstraintGraph::do_add_edge(const GNode &u,const GNode &v){
	set<GNode> & lu = access_list(u);
	set<GNode> & lv = access_list(v);
	lu.insert(v);
	lv.insert(u); // symmetric
}

// add an edge between node u-v, with duplicate detection, 
// regardless the color
bool ConstraintGraph::add_edge(const GNode &u,const GNode &v){
	// avoid duplicate edge
	if( has_edge(u,v) ) return false;
	do_add_edge(u,v);
	return true;
}

// try to insert edge, but color awareness
// return EXIST if the edge exists already
ADD_EDGE_RESULT ConstraintGraph::add_edge_color(const GNode &u,const GNode &v){
	if( has_edge(u,v) ) return EXIST;
	// check if adding this edge will conflict
	// explore all 9 cases: {G,H,V} X {G,H,V}
	
	set<GNode> & lu = access_list(u);
	set<GNode> & lv = access_list(v);
	COLOR ucolor = get_color(u);
	COLOR vcolor = get_color(v);
	
	if( lu.empty() ){// node u standalone
		do_add_edge(u,v); // safely add edge
		if( lv.empty() ){ // G,G: color them
			recur_color(u,H);
		}
		else if( vcolor == H ){// G,H: color u to L
			set_color(u,L);
		}
		else{// vcolor == L
			set_color(u,H);
		}
		return SUCCESS;
	}
	else if( lv.empty() ) {// v standlone(symmetric)
		do_add_edge(v,u); // safely add edge
		if( lu.empty() ){ // G,G: color them
			recur_color(v,H);
		}
		else if( ucolor == H ){// H,G: color v to L
			set_color(v,L);
		}
		else{// vcolor == L
			set_color(v,H);
		}
		return SUCCESS;
	}
	else{// u,v not standalone
		if( ucolor != vcolor ){// H-L or L-H
			do_add_edge(u,v);
			return SUCCESS;
		}
		// H-H or L-L
		// IMPORTANT: not possible in the same component
		// it might be that by swapping the value
		// of one connected component, we can do 2-coloring
		ConstraintGraph bak(*this); // make backup
		reverse_color(v);
		COLOR u_newcolor = get_color(u);
		if( ucolor != u_newcolor ){
			*this = bak;
			return FAIL;
		}
		else
			return SUCCESS;
	}
}

// recursively reverse the color of a componenet, which containing `node'
void ConstraintGraph::recur_reverse_color(const GNode &node,
		BoolVector &r_mark,BoolVector &c_mark){
	if(node.type == ROW){
		if( r_mark[node.idx] == true) return;
		else r_mark[node.idx] = true;
	}
	if(node.type == COL) {
		if( c_mark[node.idx] == true) return;
		else c_mark[node.idx] = true;
	}
	COLOR origin_color = get_color(node);
	COLOR new_color = next_color(origin_color);
	set_color(node,new_color);

	GNodeSet & l = access_list(node);
	GNodeSet::iterator it;
	for (it = l.begin(); it != l.end(); it++)
		recur_reverse_color(*it,r_mark,c_mark);
}

// reverse the color of a component, which containing `node'
void ConstraintGraph::reverse_color(const GNode &node){
	BoolVector r_mark(row,false);
	BoolVector c_mark(col,false);
	recur_reverse_color(node,r_mark,c_mark);
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

bool operator <(GNode u,GNode v){
	return (u.type == ROW && v.type == COL) ||
		(u.type == v.type && u.idx < v.idx);
}