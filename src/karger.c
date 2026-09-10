#include "karger.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Fisher-Yates shuffle of edge indices (in place into perm). */
static void shuffle_edges(const edge_graph_t *g, rng_t *rng, igraph_integer_t *perm)
{
    for (igraph_integer_t i = 0; i < g->m; i++) perm[i] = i;
    for (igraph_integer_t i = g->m - 1; i > 0; i--) {
        igraph_integer_t j = (igraph_integer_t)rng_choice(rng, (uint64_t)i + 1);
        igraph_integer_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
    }
}

igraph_integer_t karger_trial(const edge_graph_t *g, rng_t *rng, bool *side)
{
    igraph_integer_t n = g->n;
    if (n <= 1) {
        for (igraph_integer_t v = 0; v < n; v++) side[v] = true;
        return 0;
    }

    uf_t uf;
    uf_init(&uf, n);
    igraph_integer_t *perm = xmalloc((size_t)g->m * sizeof(igraph_integer_t));
    shuffle_edges(g, rng, perm);

    /* contract along the random order until 2 supernodes remain */
    igraph_integer_t comps = n;
    for (igraph_integer_t i = 0; i < g->m && comps > 2; i++) {
        igraph_integer_t a = uf_find(&uf, g->u[perm[i]]);
        igraph_integer_t b = uf_find(&uf, g->v[perm[i]]);
        if (a != b) { uf_union(&uf, a, b); comps--; }
    }

    /* cut = edges crossing the two final supernodes */
    igraph_integer_t cut = 0;
    for (igraph_integer_t i = 0; i < g->m; i++) {
        igraph_integer_t a = uf_find(&uf, g->u[i]);
        igraph_integer_t b = uf_find(&uf, g->v[i]);
        if (a != b) cut++;
    }

    igraph_integer_t c0 = uf_find(&uf, 0);
    for (igraph_integer_t v = 0; v < n; v++)
        side[v] = (uf_find(&uf, v) == c0);

    free(perm);
    uf_destroy(&uf);
    return cut;
}

igraph_integer_t karger_mincut(const edge_graph_t *g, rng_t *rng,
                               igraph_integer_t trials, bool *best_side)
{
    igraph_integer_t best = g->m + 1;
    bool *side = xmalloc((size_t)g->n * sizeof(bool));
    for (igraph_integer_t t = 0; t < trials; t++) {
        igraph_integer_t c = karger_trial(g, rng, side);
        if (c < best) {
            best = c;
            for (igraph_integer_t v = 0; v < g->n; v++) best_side[v] = side[v];
        }
    }
    free(side);
    return best;
}

igraph_integer_t karger_brute_mincut(const edge_graph_t *g)
{
    igraph_integer_t n = g->n;
    if (n <= 1) return 0;
    if (n > 24) {
        fprintf(stderr, "brute-force mincut: n=%" IGRAPH_PRId " too large\n", n);
        exit(1);
    }
    igraph_integer_t best = g->m + 1;
    /* fix vertex 0 on side A; enumerate all subsets of the others,
     * skipping the partition with an empty other side */
    uint64_t total = (uint64_t)1 << (n - 1);
    for (uint64_t mask = 0; mask < total - 1; mask++) {
        igraph_integer_t cut = 0;
        for (igraph_integer_t i = 0; i < g->m; i++) {
            igraph_integer_t a = g->u[i], b = g->v[i];
            bool sa = (a == 0) ? true : ((mask >> (a - 1)) & 1);
            bool sb = (b == 0) ? true : ((mask >> (b - 1)) & 1);
            if (sa != sb) cut++;
        }
        if (cut < best) best = cut;
    }
    return best;
}

/* --- Karger-Stein: recursive contraction with 2 independent runs --- */

/* Contract `g` down to `t` supernodes (random order), producing `out`. */
static void do_contract(const edge_graph_t *g, igraph_integer_t t, rng_t *rng,
                        edge_graph_t *out)
{
    igraph_integer_t n = g->n;
    uf_t uf;
    uf_init(&uf, n);
    igraph_integer_t *perm = xmalloc((size_t)g->m * sizeof(igraph_integer_t));
    shuffle_edges(g, rng, perm);

    igraph_integer_t comps = n;
    for (igraph_integer_t i = 0; i < g->m && comps > t; i++) {
        igraph_integer_t a = uf_find(&uf, g->u[perm[i]]);
        igraph_integer_t b = uf_find(&uf, g->v[perm[i]]);
        if (a != b) { uf_union(&uf, a, b); comps--; }
    }

    /* rename supernodes 0..k-1 */
    igraph_integer_t *id = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t k = 0;
    for (igraph_integer_t v = 0; v < n; v++) id[v] = -1;
    igraph_integer_t edges = 0;
    for (igraph_integer_t i = 0; i < g->m; i++) {
        igraph_integer_t a = uf_find(&uf, g->u[i]);
        igraph_integer_t b = uf_find(&uf, g->v[i]);
        if (a == b) continue;
        if (id[a] == -1) id[a] = k++;
        if (id[b] == -1) id[b] = k++;
        edges++;
    }

    out->n = k;
    out->m = edges;
    out->u = xmalloc((size_t)(edges ? edges : 1) * sizeof(igraph_integer_t));
    out->v = xmalloc((size_t)(edges ? edges : 1) * sizeof(igraph_integer_t));
    igraph_integer_t e = 0;
    for (igraph_integer_t i = 0; i < g->m; i++) {
        igraph_integer_t a = uf_find(&uf, g->u[i]);
        igraph_integer_t b = uf_find(&uf, g->v[i]);
        if (a == b) continue;
        out->u[e] = id[a];
        out->v[e] = id[b];
        e++;
    }

    free(id);
    free(perm);
    uf_destroy(&uf);
}

igraph_integer_t karger_stein(const edge_graph_t *g, rng_t *rng)
{
    if (g->n <= 6) return karger_brute_mincut(g);

    igraph_integer_t t = (igraph_integer_t)ceil((double)g->n / sqrt(2.0));
    edge_graph_t g1, g2;
    do_contract(g, t, rng, &g1);
    do_contract(g, t, rng, &g2);
    igraph_integer_t c1 = karger_stein(&g1, rng);
    igraph_integer_t c2 = karger_stein(&g2, rng);
    edge_graph_destroy(&g1);
    edge_graph_destroy(&g2);
    return c1 < c2 ? c1 : c2;
}
