#include <iomanip>
#include <iostream>
#include "GridPoint.h"
using std::endl;
using std::setw;
using std::ios;

int GridPoint::counter=0;

GridPoint::GridPoint(Point pt_,GridPoint *par,
		int t,int l,double c,int b,int s,int d):
	pt(pt_),parent(par),time(t),length(l),cell(c),bend(b),
	stalling(s),distance(d){
		order=counter++;
		updateWeight();
}


bool GridPoint::operator == (const GridPoint &g) const{
	//return weight == g.weight;
	return GT(weight,g.weight);
}

double GridPoint::updateWeight(){
	double old = weight;
	weight = time+length+cell+bend+stalling+distance;
	return old;
}	

ostream & operator <<(ostream &out,const GridPoint & g){
	out<<"t="<<setw(2)<<std::left<<g.time<<" at "
		 <<std::left<<g.pt<<",\tw="
		 <<setw(3)<<g.distance<<" +"
		 <<setw(3)<<g.time<<" +"
		 <<setw(3)<<g.length<<" +"
		 <<setw(3)<<g.cell<<" ="
		 <<setw(3)<<std::left<<g.weight<<", par=";
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


/*
bool GridPoint::operator < (const GridPoint& g) const{ 
	if( weight == g.weight ) 
		if( time == 
	return weight < g.weight; 
}
*/
