#ifndef HUNGARIAN_H
#define HUNGARIAN_H

#include <igraph.h>

/* Hungarian algorithm (assignment problem) on a square cost matrix.
 * cost is row-major n*n and must contain only finite values (the CLI maps
 * non-edges to 0 before calling).  Fills assignment[row] = column assigned
 * to row.  Returns the total cost of the assignment.  Minimization.
 * (For max-weight matching, negate the weights before calling.) */
igraph_real_t hungarian_solve(const igraph_real_t *cost, igraph_integer_t n,
                              igraph_integer_t *assignment);

#endif
