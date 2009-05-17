#ifndef __DRAW_VOLTAGE_H__
#define __DRAW_VOLTAGE_H__
#include "Router.h"

void begin_figure(FILE * fig);
void end_figure(FILE * fig,int time);
void draw_voltage(const RouteResult & result,const Chip & chip,
    const char * filename);
#endif
