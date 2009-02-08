#include "header.h"

ostream & operator <<(ostream & out,Point & pt){
	out<<"("<<pt.x<<","<<pt.y<<")";
	return out;
}


