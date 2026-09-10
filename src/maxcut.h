#ifndef MAXCUT_H
#define MAXCUT_H

#include "edgegraph.h"
#include "util.h"

/* Randomized max cut: every vertex joins side 1 with probability 1/2.
 * side[v] is filled; returns the cut value (number of crossing edges). */
igraph_integer_t maxcut_random(const edge_graph_t *g, rng_t *rng, bool *side);

/* Best of `trials` independent random cuts. */
igraph_integer_t maxcut_best(const edge_graph_t *g, rng_t *rng,
                             igraph_integer_t trials, bool *best_side);

/* Deterministic helper: cut value of a side assignment. */
igraph_integer_t maxcut_value(const edge_graph_t *g, const bool *side);
#endif
