#include "uf.h"

#include "util.h"

void uf_init(uf_t *u, igraph_integer_t n)
{
    u->n = n;
    u->parent = xmalloc((size_t)n * sizeof(igraph_integer_t));
    u->rank   = xcalloc((size_t)n, sizeof(igraph_integer_t));
    for (igraph_integer_t i = 0; i < n; i++) u->parent[i] = i;
}

igraph_integer_t uf_find(uf_t *u, igraph_integer_t x)
{
    while (u->parent[x] != x) {
        u->parent[x] = u->parent[u->parent[x]];   /* path halving */
        x = u->parent[x];
    }
    return x;
}

void uf_union(uf_t *u, igraph_integer_t a, igraph_integer_t b)
{
    igraph_integer_t ra = uf_find(u, a), rb = uf_find(u, b);
    if (ra == rb) return;
    if (u->rank[ra] < u->rank[rb]) {
        u->parent[ra] = rb;
    } else {
        u->parent[rb] = ra;
        if (u->rank[ra] == u->rank[rb]) u->rank[ra]++;
    }
}

void uf_destroy(uf_t *u)
{
    free(u->parent);
    free(u->rank);
    u->parent = NULL;
    u->rank = NULL;
}
