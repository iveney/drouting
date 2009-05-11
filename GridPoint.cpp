#include <iomanip>
#include <iostream>
#include "GridPoint.h"
using std::endl;
using std::setw;
using std::ios;

int GridPoint::counter=0;

GridPoint::GridPoint(Point pt_,GridPoint *par,
		int t,int b,int f,int e,int s,int d):
	pt(pt_),parent(par),time(t),bend(b),
	fluidic(f),electro(e),stalling(s),distance(d){
		order=counter++;
		updateWeight();
}

/*
bool GridPoint::operator < (const GridPoint& g) const{ 
	if( weight == g.weight ) 
		if( time == 
	return weight < g.weight; 
}
*/

bool GridPoint::operator == (const GridPoint &g) const{
	return weight == g.weight;
}

int GridPoint::updateWeight(){
	int old = weight;
	weight = time+bend+fluidic+electro+stalling+distance;
	return old;
}	

ostream & operator <<(ostream &out,const GridPoint & g){
	out<<"t="<<setw(2)<<std::left<<g.time<<" at "
		 <<std::left<<g.pt<<",\tw="
		 <<setw(3)<<std::left<<g.weight<<"\t MHT="
		 <<setw(3)<<std::left<<g.distance<<", par=";
	if( g.parent == NULL ) out<<"NULL";
	else out<<g.parent->pt;
	return out;
}

/*
ostream & operator <<(ostream &out,const GridPoint *g){
	out<<*g;
	return out;
}
*/
