#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "header.h"

/* parser.c */
const char *getColor(void);
Chip *parse(FILE *f, Chip *chip);
void drawSubproblem(Subproblem *prob, int N, int M, int num, char *name);

#endif
