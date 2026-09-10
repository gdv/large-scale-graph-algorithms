#include "matching.h"

#include "flow.h"
#include "util.h"

igraph_integer_t matching_hopcroft_karp(const csr_t *g, const bool *side,
                                        igraph_integer_t *match_l,
                                        igraph_integer_t *match_r)
{
    (void)g; (void)side;
    for (igraph_integer_t v = 0; v < g->n; v++) match_l[v] = match_r[v] = -1;
    return 0;
}

igraph_integer_t matching_via_flow(const csr_t *g, const bool *side,
                                   igraph_integer_t *match_l,
                                   igraph_integer_t *match_r)
{
    (void)g; (void)side;
    for (igraph_integer_t v = 0; v < g->n; v++) match_l[v] = match_r[v] = -1;
    return 0;
}
