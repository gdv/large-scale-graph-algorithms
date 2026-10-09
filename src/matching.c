#include "matching.h"

#include "flow.h"
#include "util.h"

/* ---------------- Hopcroft-Karp ---------------- */

/* BFS layering over alternating paths, starting from all free L-vertices.
 * dist[u] = layer of L-vertex u (INF = not layered).  Returns the shortest
 * distance to a free R-vertex (INF if none). */
static igraph_integer_t hk_bfs(const csr_t *g, const bool *side,
                               const igraph_integer_t *match_l,
                               const igraph_integer_t *match_r,
                               igraph_integer_t *dist, igraph_integer_t *q)
{
    igraph_integer_t n = g->n;
    igraph_integer_t INF = n + 1;
    igraph_integer_t qh = 0, qt = 0, shortest = INF;

    for (igraph_integer_t u = 0; u < n; u++) {
        dist[u] = INF;
        if (side[u] && match_l[u] == -1) {
            dist[u] = 0;
            q[qt++] = u;
        }
    }
    while (qh < qt) {
        igraph_integer_t u = q[qh++];
        if (dist[u] >= shortest) continue;      /* deeper layers are useless */
        for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++) {
            igraph_integer_t w = g->targets[a];
            if (side[w]) continue;              /* only cross arcs */
            igraph_integer_t u2 = match_r[w];
            if (u2 == -1) {
                if (dist[u] + 1 < shortest) shortest = dist[u] + 1;
            } else if (dist[u2] == INF) {
                dist[u2] = dist[u] + 1;
                q[qt++] = u2;
            }
        }
    }
    return shortest;
}

static bool hk_dfs(const csr_t *g, const bool *side, igraph_integer_t *dist,
                   igraph_integer_t *match_l, igraph_integer_t *match_r,
                   igraph_integer_t u, igraph_integer_t limit)
{
    for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++) {
        igraph_integer_t w = g->targets[a];
        if (side[w]) continue;
        igraph_integer_t u2 = match_r[w];
        if (u2 == -1) {
            if (dist[u] + 1 == limit) {         /* augmenting path of shortest length */
                match_l[u] = w;
                match_r[w] = u;
                return true;
            }
        } else if (dist[u2] == dist[u] + 1 &&
                   hk_dfs(g, side, dist, match_l, match_r, u2, limit)) {
            match_l[u] = w;
            match_r[w] = u;
            return true;
        }
    }
    dist[u] = g->n + 1;                         /* prune dead ends */
    return false;
}

igraph_integer_t matching_hopcroft_karp(const csr_t *g, const bool *side,
                                        igraph_integer_t *match_l,
                                        igraph_integer_t *match_r)
{
    igraph_integer_t n = g->n;
    igraph_integer_t *dist = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) match_l[v] = match_r[v] = -1;

    igraph_integer_t size = 0;
    for (;;) {
        igraph_integer_t limit = hk_bfs(g, side, match_l, match_r, dist, q);
        if (limit == n + 1) break;              /* no augmenting path left */
        igraph_integer_t added = 0;
        for (igraph_integer_t u = 0; u < n; u++)
            if (side[u] && match_l[u] == -1 && dist[u] == 0 &&
                hk_dfs(g, side, dist, match_l, match_r, u, limit))
                added++;
        if (added == 0) break;                  /* safety net */
        size += added;
    }

    xfree(dist);
    xfree(q);
    return size;
}

/* ---------------- Matching via max flow (Dinic) ---------------- */

igraph_integer_t matching_via_flow(const csr_t *g, const bool *side,
                                   igraph_integer_t *match_l,
                                   igraph_integer_t *match_r)
{
    igraph_integer_t n = g->n;
    for (igraph_integer_t v = 0; v < n; v++) match_l[v] = match_r[v] = -1;

    igraph_integer_t nL = 0, cross = 0;
    for (igraph_integer_t u = 0; u < n; u++)
        if (side[u]) {
            nL++;
            for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++)
                if (!side[g->targets[a]]) cross++;
        }

    igraph_integer_t S = n, T = n + 1;
    flow_t f;
    flow_init(&f, n + 2, nL + cross + (n - nL));

    for (igraph_integer_t u = 0; u < n; u++)
        if (side[u]) flow_add_edge(&f, S, u, 1.0);
    for (igraph_integer_t u = 0; u < n; u++)
        for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++) {
            igraph_integer_t w = g->targets[a];
            if (side[u] && !side[w]) flow_add_edge(&f, u, w, 1.0);
        }
    for (igraph_integer_t v = 0; v < n; v++)
        if (!side[v]) flow_add_edge(&f, v, T, 1.0);

    igraph_real_t val = flow_dinic(&f, S, T, NULL);

    /* edge k is the (k - nL)-th L->R edge; flow on its forward arc == 1
     * iff it belongs to the matching */
    igraph_integer_t k = 0;
    for (igraph_integer_t u = 0; u < n; u++) if (side[u]) k++;      /* S->L */
    for (igraph_integer_t u = 0; u < n; u++)
        for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++) {
            igraph_integer_t w = g->targets[a];
            if (side[u] && !side[w]) {
                if (f.cap[2 * k + 1] > 0.5) { match_l[u] = w; match_r[w] = u; }
                k++;
            }
        }

    flow_destroy(&f);
    return (igraph_integer_t)val;
}
