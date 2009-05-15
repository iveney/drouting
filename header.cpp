#include "header.h"

ostream & operator <<(ostream & out,const Point & pt){
	out<<"("<<pt.x<<","<<pt.y<<")";
	return out;
}

// swap two integers
void swap(int &a,int &b){
	int t=a;
	a=b;
	b=t;
}

// get the bounding box of two nets
Block get_bbox(const Pin & p1, const Pin & p2){
	Block bb;
	bb.pt[LL].x = MIN(p1.pt.x,p2.pt.x);
	bb.pt[LL].y = MIN(p1.pt.y,p2.pt.y);
	bb.pt[UR].x = MAX(p1.pt.x,p2.pt.x);
	bb.pt[UR].y = MAX(p1.pt.y,p2.pt.y);
	return bb;
}

// determine if a point is inside a rect
bool pt_in_rect(const Block & bb, const Point & pt){
	if( pt.x<=bb.pt[UR].x && pt.x>=bb.pt[LL].x &&
	    pt.y<=bb.pt[UR].y && pt.y>=bb.pt[LL].y)
		return true;
	return false;
}


bool operator < (const Point &a, const Point &b){
	if( a.x == b.x )
		return a.y < b.y;
	else
		return a.x < b.x;
}

