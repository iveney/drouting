#ifndef __UTIL_H__
#define __UTIL_H__

enum DIRECTION{LEFT,RIGHT,UP,DOWN,STAY}; // LEFT, RIGHT, UP, DOWN

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define report_exit(a) _report_exit(AT,a)

void _report_exit(const char *location, const char *msg);
bool in_grid(const Point & pt);
vector<Point> get_neighbour(const Point & pt);
bool check_bending(const Point & p1,const Point & p2);
DIRECTION pt_relative_pos(const Point & l ,const Point & r);

#endif
