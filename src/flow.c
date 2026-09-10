#include "flow.h"

#include "util.h"

void flow_init(flow_t *f, igraph_integer_t n, igraph_integer_t m)
{
    f->n = n;
    f->m = m;
    f->e = 0;
    f->head = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t i = 0; i < n; i++) f->head[i] = -1;
    f->next = xmalloc((size_t)(2 * m) * sizeof(igraph_integer_t));
    f->to   = xmalloc((size_t)(2 * m) * sizeof(igraph_integer_t));
    f->cap  = xmalloc((size_t)(2 * m) * sizeof(igraph_real_t));
}

void flow_add_edge(flow_t *f, igraph_integer_t from, igraph_integer_t to,
                   igraph_real_t cap)
{
    igraph_integer_t a = 2 * f->e++;
    f->to[a] = to;
    f->cap[a] = cap;
    f->next[a] = f->head[from];
    f->head[from] = a;

    f->to[a ^ 1] = from;
    f->cap[a ^ 1] = 0.0;
    f->next[a ^ 1] = f->head[to];
    f->head[to] = a ^ 1;
}

void flow_destroy(flow_t *f)
{
    free(f->head); free(f->next); free(f->to); free(f->cap);
    f->head = f->next = f->to = NULL;
    f->cap = NULL;
}

igraph_real_t flow_ford_fulkerson(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                                  igraph_real_t *flow)
{
    (void)f; (void)s; (void)t; (void)flow;
    return 0.0;
}

igraph_real_t flow_edmonds_karp(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                                igraph_real_t *flow)
{
    (void)f; (void)s; (void)t; (void)flow;
    return 0.0;
}

igraph_real_t flow_dinic(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                         igraph_real_t *flow)
{
    (void)f; (void)s; (void)t; (void)flow;
    return 0.0;
}

igraph_real_t flow_preflow_push(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                                igraph_real_t *flow)
{
    (void)f; (void)s; (void)t; (void)flow;
    return 0.0;
}

bool *flow_mincut_side(const flow_t *f, igraph_integer_t s)
{
    (void)f; (void)s;
    return NULL;
}
