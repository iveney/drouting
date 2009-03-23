#include "header.h"

ostream & operator <<(ostream & out,Point & pt){
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
Block getBoundingBox(const Pin & p1, const Pin & p2){
	Block bb;
	bb.pt[LL].x = MIN(p1.pt.x,p2.pt.x);
	bb.pt[LL].y = MIN(p1.pt.y,p2.pt.y);
	bb.pt[UR].x = MAX(p1.pt.x,p2.pt.x);
	bb.pt[UR].y = MAX(p1.pt.y,p2.pt.y);
	return bb;
}

// determine if a point is inside a rect
bool ptInRect(const Block & bb, const Point & pt){
	if( pt.x<=bb.pt[UR].x && pt.x>=bb.pt[LL].x &&
	    pt.y<=bb.pt[UR].y && pt.y>=bb.pt[LL].y)
		return true;
	return false;
}

