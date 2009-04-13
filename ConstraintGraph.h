#ifndef __CONSTRAINT_GRAPH__
#define __CONSTRAINT_GRAPH__

#include <vector>
#include <set>
//#include <cassert>
using std::set;
using std::vector;

enum GType{ROW,COL};

class GNode{
public:
	friend bool operator <(GNode u,GNode v);
	void set(GType t,int index){
		type = t;
		idx = index;
	}
	
	GType type;
	int idx;
};

bool operator <(GNode u,GNode v){
	return (u.type == ROW && v.type == COL) ||
		(u.type == v.type && u.idx < v.idx);
}

class ConstraintGraph{ 
public:
	ConstraintGraph(int r,int c):row(r),col(c),
	r_list(vector< set<GNode> >(row)),
	c_list(vector< set<GNode> >(col))
	{}

	bool addEdge(GNode u,GNode v){
		set<GNode> & lu = 
			(u.type==ROW?r_list:c_list)[u.idx];
		set<GNode> & lv = 
			(v.type==ROW?r_list:c_list)[v.idx];
		// avoid duplicate edge
		if( lu.find(v) != lu.end() )
			return false;
		lu.insert(v);
		lv.insert(u);
		return true;
	}

	int row,col;  // col_index = row_index + col
	vector< set<GNode> > r_list;
	vector< set<GNode> > c_list;
};

#endif
