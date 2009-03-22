#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <vector>
#include "header.h"
#include "parser.h"

enum DIRECTION{LEFT,RIGHT,UP,DOWN,STAY}; // LEFT, RIGHT, UP, DOWN

bool electrodeCheck(const Point & pt);
bool inGrid(const Point & pt);
std::vector<Point> getNbr(const Point & pt);
void initBlock(Subproblem *p);
void sortNet(Subproblem * p, int * netorder);
Chip * read_file(int argc, char * argv[], Chip * chip);
Point traceback_line(int which, int t, const Point & current, DIRECTION dir);
DIRECTION PtRelativePos(const Point & l ,const Point & r);
void traceback(int which, Point & current);
int fluidicCheck(int which, const Point & pt,int t);
bool checkBending(const Point & p1,const Point & p2);

#endif
