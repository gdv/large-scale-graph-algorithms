#include "hungarian.h"

#include "util.h"

/* Classic Hungarian with potentials (u, v) and an alternating tree
 * (way[]).  Written 1-based internally; the caller's 0-based cost matrix
 * is copied into a[1..n][1..n].  O(n^3).  Minimization. */
igraph_real_t hungarian_solve(const igraph_real_t *cost, igraph_integer_t n,
                              igraph_integer_t *assignment)
{
    igraph_real_t *a = xmalloc((size_t)(n + 1) * (n + 1) * sizeof(igraph_real_t));
    for (igraph_integer_t i = 0; i < n; i++)
        for (igraph_integer_t j = 0; j < n; j++)
            a[(i + 1) * (n + 1) + (j + 1)] = cost[i * n + j];

    igraph_real_t *u = xcalloc((size_t)n + 1, sizeof(igraph_real_t));
    igraph_real_t *v = xcalloc((size_t)n + 1, sizeof(igraph_real_t));
    igraph_real_t *minv = xcalloc((size_t)n + 1, sizeof(igraph_real_t));
    igraph_integer_t *way = xcalloc((size_t)n + 1, sizeof(igraph_integer_t));
    igraph_integer_t *match = xcalloc((size_t)n + 1, sizeof(igraph_integer_t));
    bool *used = xmalloc((size_t)(n + 1) * sizeof(bool));

    for (igraph_integer_t i = 1; i <= n; i++) {
        match[0] = i;
        igraph_integer_t j0 = 0;
        for (igraph_integer_t j = 1; j <= n; j++) { minv[j] = IGRAPH_INFINITY; used[j] = false; }

        do {
            used[j0] = true;
            igraph_integer_t i0 = match[j0];
            igraph_real_t delta = IGRAPH_INFINITY;
            igraph_integer_t j1 = 0;
            for (igraph_integer_t j = 1; j <= n; j++) {
                if (used[j]) continue;
                igraph_real_t cur = a[i0 * (n + 1) + j] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }
            for (igraph_integer_t j = 0; j <= n; j++) {
                if (used[j]) { u[match[j]] += delta; v[j] -= delta; }
                else minv[j] -= delta;
            }
            j0 = j1;
        } while (match[j0] != 0);

        /* augment along the alternating path found */
        do {
            igraph_integer_t j1 = way[j0];
            match[j0] = match[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    for (igraph_integer_t j = 1; j <= n; j++) {
        igraph_integer_t row = match[j];
        if (row >= 1 && row <= n) assignment[row - 1] = j - 1;
    }
    igraph_real_t total = -v[0];

    free(a); free(u); free(v); free(minv); free(way); free(match); free(used);
    return total;
}
