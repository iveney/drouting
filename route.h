#ifndef __ROUTE_H__
#define __ROUTE_H__
#include <vector>
#include "header.h"
#include "parser.h"
using std::vector;

enum DIRECTION{LEFT,RIGHT,UP,DOWN,STAY}; // LEFT, RIGHT, UP, DOWN

bool electrodeCheck(const Point & pt);
bool fluidicCheck(int which, const Point & pt,int t);
bool inGrid(const Point & pt);
vector<Point> getNbr(const Point & pt);
void initBlock(Subproblem *p);
void sortNet(Subproblem * p, int * netorder);
Chip * init(int argc, char * argv[], Chip * chip);
Point traceback_line(int which, int t, const Point & current, DIRECTION dir);
DIRECTION PtRelativePos(const Point & l ,const Point & r);
void traceback(int which, Point & current);
Chip * init(int argc, char * argv[], Chip * chip);
#endif
