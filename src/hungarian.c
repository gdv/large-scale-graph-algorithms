#include "hungarian.h"

#include "util.h"

igraph_real_t hungarian_solve(const igraph_real_t *cost, igraph_integer_t n,
                              igraph_integer_t *assignment)
{
    (void)cost; (void)n;
    for (igraph_integer_t i = 0; i < n; i++) assignment[i] = -1;
    return 0.0;
}
