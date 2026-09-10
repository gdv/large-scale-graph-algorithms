#ifndef MATCHING_H
#define MATCHING_H

#include "csr.h"

#include <igraph.h>
#include <stdbool.h>

/* Maximum cardinality matching in a bipartite graph given as an
 * undirected CSR.  side[v] is the bipartition (true = "left").
 * match_l/match_r hold the partner of each vertex (-1 if unmatched),
 * size n each.  Both return the size of the computed matching. */
igraph_integer_t matching_hopcroft_karp(const csr_t *g, const bool *side,
                                        igraph_integer_t *match_l,
                                        igraph_integer_t *match_r);

igraph_integer_t matching_via_flow(const csr_t *g, const bool *side,
                                   igraph_integer_t *match_l,
                                   igraph_integer_t *match_r);

#endif
