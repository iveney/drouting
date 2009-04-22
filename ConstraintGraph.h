#ifndef __CONSTRAINT_GRAPH__
#define __CONSTRAINT_GRAPH__

#include <vector>
#include <set>
using std::set;
using std::vector;

static const char * color_string[]={"H","L","G"};

class GNode;
class GEdge;
//typedef set<GNode> GNodeSet;
//typedef set<GEdge> GEdgeSet;
typedef vector<bool> BoolVector;
typedef vector<GNode> GNodeVector;
typedef vector<GEdge> GEdgeVector;

enum NType{ROW,COL};
enum EType{SAME,DIFF,NOEDGE};
enum COLOR{HI,LO,G};
enum ADD_EDGE_RESULT{FAIL,SUCCESS,EXIST};

class GNode{
public:
	friend bool operator <(GNode u,GNode v);
	friend bool operator == (GNode u,GNode v);
	friend bool operator != (GNode u, GNode v);

	GNode(){}

	// default color is G
	GNode(NType t,int index,COLOR clr=G):
	      type(t),idx(index),color(clr),ecount(0){}

	void set(NType t,int index){
		type = t;
		idx = index;
	}
	
	NType type;  // specify the node type
	int idx;     // which index of this node
	COLOR color; //

	// how many edges from this node,should not be used directly
	int ecount;  
};

class GEdge{
public: 
	//friend bool operator <(GEdge e1,GEdge e2);

	//default constructor
	GEdge():type(NOEDGE),count(0){
	}
	GEdge(const GNode &u_,const GNode &v_,
	      EType t=NOEDGE):u(u_),v(v_),type(t),count(0){}

	bool operator == (const GEdge & edge){
		return (u==edge.u && u==edge.v) ||
		       (u==edge.v && v==edge.u);
	}

	GNode u,v;
	// SAME or DIFF, note being BOTH type means conflict
	EType type;  
	// keep track of how many conflict edges
	int count;
};

class ConstraintGraph{ 
public:
	friend class Router;

	// given row number and column number, initialize the graph

	ConstraintGraph(int width,int height);
	~ConstraintGraph();

	////////////////////////////////////////////////////////
	bool has_edge(const GNode &u,const GNode &v);
	bool remove_edge(const GNode & u,const GNode & v);
	COLOR erase_color(const GNode & node);
	bool add_edge_color(const GNode &u,const GNode &v,EType type);
	bool recur_color(const GNode & node,COLOR assign);
	bool try_coloring();

private:
	bool do_add_edge(const GNode &u,const GNode &v,EType type);
	void recur_reverse_color(const GNode &node, BoolVector &mark);
	void reverse_color(const GNode &node);

	int row,col;                // row and col count
	GNodeVector node_list;    // 0~row-1 is ROW
				    // row~(row+col-1) is col
	vector< GEdgeVector > edge;
};

#endif
