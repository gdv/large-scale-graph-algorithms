#ifndef KARGER_H
#define KARGER_H

#include "edgegraph.h"
#include "uf.h"
#include "util.h"

/* One contraction trial.  Returns the cut size and fills side[] (one side
 * of the cut = the component of vertex 0). */
igraph_integer_t karger_trial(const edge_graph_t *g, rng_t *rng, bool *side);

/* Best cut over `trials` trials. */
igraph_integer_t karger_mincut(const edge_graph_t *g, rng_t *rng,
                               igraph_integer_t trials, bool *best_side);

/* Karger-Stein recursion (returns the cut value). */
igraph_integer_t karger_stein(const edge_graph_t *g, rng_t *rng);

/* Exact min cut by brute force (2^(n-1) partitions; use for small n);
 * test oracle and base case for Karger-Stein (n <= 6). */
igraph_integer_t karger_brute_mincut(const edge_graph_t *g);
#endif
