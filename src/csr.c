#include "csr.h"

#include "util.h"

void csr_build(csr_t *g, const igraph_t *graph, bool directed)
{
    igraph_integer_t n = (igraph_integer_t)igraph_vcount(graph);
    igraph_integer_t m = (igraph_integer_t)igraph_ecount(graph);
    igraph_integer_t arcs = directed ? m : 2 * m;

    g->n = n;
    g->m = arcs;
    g->offsets = xcalloc((size_t)n + 1, sizeof(igraph_integer_t));
    g->targets = xmalloc((size_t)arcs * sizeof(igraph_integer_t));
    g->rev     = directed ? NULL : xmalloc((size_t)arcs * sizeof(igraph_integer_t));

    igraph_vector_int_t elist;
    igraph_vector_int_init(&elist, 0);
    /* bycol=false: res is the flat sequence (from,to) of each edge */
    igraph_get_edgelist(graph, &elist, false);

    for (igraph_integer_t i = 0; i < m; i++) {
        g->offsets[VECTOR(elist)[2 * i]]++;
        if (!directed) g->offsets[VECTOR(elist)[2 * i + 1]]++;
    }
    igraph_integer_t acc = 0;
    for (igraph_integer_t u = 0; u < n; u++) {
        igraph_integer_t deg = g->offsets[u];
        g->offsets[u] = acc;
        acc += deg;
    }
    g->offsets[n] = acc;

    igraph_integer_t *pos = xmalloc((size_t)n * sizeof(igraph_integer_t));   /* running write cursor per vertex */
    for (igraph_integer_t u = 0; u < n; u++) pos[u] = g->offsets[u];

    for (igraph_integer_t i = 0; i < m; i++) {
        igraph_integer_t from = VECTOR(elist)[2 * i];
        igraph_integer_t to   = VECTOR(elist)[2 * i + 1];
        igraph_integer_t a    = pos[from]++;
        g->targets[a] = to;
        if (!directed) {
            igraph_integer_t b = pos[to]++;
            g->targets[b] = from;
            g->rev[a] = b;
            g->rev[b] = a;
        }
    }

    free(pos);
    igraph_vector_int_destroy(&elist);
}

void csr_destroy(csr_t *g)
{
    free(g->offsets);
    free(g->targets);
    free(g->rev);
    g->offsets = NULL;
    g->targets = NULL;
    g->rev     = NULL;
    g->n = 0;
    g->m = 0;
}
