#ifndef GMCFUNS
#define GMCFUNS

#include "matrix.h"
#include "heap.h"
#include <cblas.h>
#include <lapacke.h>
#include <math.h>

#define EPS 2.2204e-16

matrix sqrDist(matrix m);
matrix initSIG(matrix x, uint k);
matrix updateU(matrix q);
void updateF(matrix F, matrix U, double * ev, uint c);
int connectedComp(matrix m, int * y);

#endif