#include "GridPoint.h"

int GridPoint::counter=0;

int GridPoint::updateWeight(){
	int old = weight;
	weight = time+bend+fluidic+electro+stalling+distance;
	return old;
}	
