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
static const char * color_string[]={"H","L","G"};

class GNode{
public:
	friend bool operator <(GNode u,GNode v);
	GNode(){}
	GNode(GType t,int index):type(t),idx(index){}
	void set(GType t,int index){
		type = t;
		idx = index;
	}
	
	bool operator == (const GNode & node){
		return type == node.type && idx == node.idx;
	}
	GType type;
	int idx;
};

class GEdge{
public: 
	GEdge(const GNode &u_,const GNode &v_):u(u_),v(v_){}
	bool operator == (const GEdge & edge){
		return (u==edge.u && u==edge.v) ||
			(u==edge.v && v==edge.u);
	}
	friend bool operator <(GEdge e1,GEdge e2);
	GNode u,v;
};

class ConstraintGraph{ 
public:
	friend class Router;
	// given row number and column number, initialize the graph
	ConstraintGraph(int r,int c):row(r),col(c),
	r_list(vector< GNodeSet >(row)),
	c_list(vector< GNodeSet >(col)),
	r_color(vector<COLOR>(row,G)),
	c_color(vector<COLOR>(col,G)){
		// initialize all color to G
	}

	~ConstraintGraph(){ }

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
