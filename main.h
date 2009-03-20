#ifndef __MAIN_H__
#define __MAIN_H__

BYTE blockage[MAXGRID][MAXGRID];	// Blockage bitmap
Chip chip;				// Chip data and subproblem
Grid grid[MAXNET][MAXGRID][MAXGRID];	// record the routes, grid[i][x][y]: the time step that net i occupies (x,y)
int N,M;				// row/column count
int netorder[MAXNET];			// net routing order
int netcount;				// current subproblem's net count
Point path[MAXNET][MAXTIME];		// the routing path : path[i][t]: net i's position at time t
int idx;				// problem to solve

#endif
