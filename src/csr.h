#ifndef CSR_H
#define CSR_H

#include <igraph.h>

#include <stdbool.h>

/* Compressed Sparse Row graph: the canonical large-scale representation.
 *
 *   offsets[0..n]   prefix sums of out-degrees; arcs of vertex u are
 *                   targets[offsets[u] .. offsets[u+1]-1]
 *   targets[0..m)   arc endpoints
 *   rev[0..m)       for undirected graphs, rev[a] is the id of the sibling
 *                   arc of arc a (both directions of the same edge);
 *                   NULL for directed graphs.  A self-loop's two arcs
 *                   live inside the same vertex block, so the sibling arc
 *                   need not be in the "neighbor's" block.
 *
 * Memory is exactly (n+1) + m + (m if undirected) integers: cheap, dense,
 * cache-friendly, and easy to draw on a whiteboard.
 */
typedef struct {
    igraph_integer_t n;
    igraph_integer_t m;            /* number of arcs */
    igraph_integer_t *offsets;     /* size n+1 */
    igraph_integer_t *targets;     /* size m */
    igraph_integer_t *rev;         /* size m, or NULL if directed */
} csr_t;

/* Build a CSR from an igraph graph. directed=false duplicates each edge
 * into two arcs and fills rev[].  Self-loops and parallel arcs are kept. */
void csr_build(csr_t *g, const igraph_t *graph, bool directed);

void csr_destroy(csr_t *g);

/* Out-degree of u (u must be in [0, n)). */
static inline igraph_integer_t csr_degree(const csr_t *g, igraph_integer_t u)
{
    return g->offsets[u + 1] - g->offsets[u];
}

#endif
