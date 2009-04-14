#ifndef __CONSTRAINT_GRAPH__
#define __CONSTRAINT_GRAPH__

#include <vector>
#include <set>
using std::set;
using std::vector;

class GNode;
typedef set<GNode> GNodeSet;
typedef vector<bool> BoolVector;

enum GType{ROW,COL};
enum COLOR{H,L,G};
enum ADD_EDGE_RESULT{FAIL,SUCCESS,EXIST};

class GNode{
public:
	friend bool operator <(GNode u,GNode v);
	GNode(){}
	GNode(GType t,int index):type(t),idx(index){}
	void set(GType t,int index){
		type = t;
		idx = index;
	}
	
	GType type;
	int idx;
};

class ConstraintGraph{ 
public:
	// given row number and column number, initialize the graph
	ConstraintGraph(int r=0,int c=0):row(r),col(c),
	r_list(vector< set<GNode> >(row)),
	c_list(vector< set<GNode> >(col)),
	r_color(vector<COLOR>(row,G)),
	c_color(vector<COLOR>(col,G)){
		// initialize all color to G
	}

	bool add_edge(const GNode &u,const GNode &v);
	ADD_EDGE_RESULT add_edge_color(const GNode &u,const GNode &v);
	bool remove_edge(const GNode & u,const GNode & v);
	bool has_edge(const GNode &u,const GNode &v);
	GNodeSet::iterator find_edge(const GNode &u,const GNode &v);
	bool try_coloring();
	bool recur_color(const GNode & node,COLOR assign);
	COLOR erase_color(const GNode & node);

private:
	void recur_reverse_color(const GNode &node,
		BoolVector &r_mark,BoolVector &c_mark);
	void reverse_color(const GNode &node);
	void do_add_edge(const GNode &u,const GNode &v);
	int row,col;  
	vector< GNodeSet > r_list;
	vector< GNodeSet > c_list;
	vector< COLOR > r_color;
	vector< COLOR > c_color;
};

#endif
