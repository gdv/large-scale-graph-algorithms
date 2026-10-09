#include "util.h"
#include "maxcut.h"

igraph_integer_t maxcut_value(const edge_graph_t *g, const bool *side)
{
    igraph_integer_t cut = 0;
    for (igraph_integer_t i = 0; i < g->m; i++)
        if (side[g->u[i]] != side[g->v[i]]) cut++;
    return cut;
}

igraph_integer_t maxcut_random(const edge_graph_t *g, rng_t *rng, bool *side)
{
    for (igraph_integer_t v = 0; v < g->n; v++)
        side[v] = (bool)rng_choice(rng, 2);
    return maxcut_value(g, side);
}

igraph_integer_t maxcut_best(const edge_graph_t *g, rng_t *rng,
                             igraph_integer_t trials, bool *best_side)
{
    igraph_integer_t best = -1;
    bool *side = xmalloc((size_t)g->n * sizeof(bool));
    for (igraph_integer_t t = 0; t < trials; t++) {
        igraph_integer_t c = maxcut_random(g, rng, side);
        if (c > best) {
            best = c;
            for (igraph_integer_t v = 0; v < g->n; v++) best_side[v] = side[v];
        }
    }
    xfree(side);
    return best;
}
