#ifndef SEARCH_H
#define SEARCH_H

#include "csr.h"

#include <stdbool.h>

/* ---------------- BFS ---------------- */
typedef struct {
    igraph_integer_t *parent;   /* size n, -1 = none */
    igraph_integer_t *dist;     /* size n, -1 = unreachable */
    igraph_integer_t *order;     /* size n, discovery order */
    igraph_integer_t  n_reached;
} bfs_result_t;

void bfs_run(const csr_t *g, igraph_integer_t source, bfs_result_t *res);
void bfs_result_destroy(bfs_result_t *res);

/* ---------------- Iterative DFS (explicit stack, like the slides) ------- */
typedef struct {
    igraph_integer_t *parent;   /* size n, -1 = none */
    igraph_integer_t *dis;      /* size n, discovery time, -1 = unvisited */
    igraph_integer_t *fin;      /* size n, finishing time */
    igraph_integer_t *comp;     /* size n, connected component id */
} dfs_result_t;

void dfs_run(const csr_t *g, igraph_integer_t source, dfs_result_t *res);
void dfs_result_destroy(dfs_result_t *res);

/* ---------------- A* ---------------- */
/* h is admissible; it must never overestimate the true remaining cost. */
typedef igraph_real_t (*heuristic_t)(igraph_integer_t v, igraph_integer_t target,
                                     void *ctx);

/* Unit-weight graph (CSR is unweighted in this module). Returns true iff
 * the target was reached; dist[] and parent[] are then valid for the
 * reached vertices. */
bool astar_run(const csr_t *g, igraph_integer_t source, igraph_integer_t target,
               heuristic_t h, void *ctx,
               igraph_real_t *dist, igraph_integer_t *parent);

#endif
