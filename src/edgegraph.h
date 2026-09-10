#ifndef EDGEGRAPH_H
#define EDGEGRAPH_H

#include <igraph.h>

#include <stdbool.h>

/* Plain edge list with parallel edges allowed: the right representation
 * for contraction algorithms (Karger) and for randomized cut counting.
 * Each entry is one EDGE (not two arcs). */
typedef struct {
    igraph_integer_t n;
    igraph_integer_t m;
    igraph_integer_t *u, *v;
} edge_graph_t;

void edge_graph_build(edge_graph_t *e, const igraph_t *graph);
void edge_graph_destroy(edge_graph_t *e);
#endif
