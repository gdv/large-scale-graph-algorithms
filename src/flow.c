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

/* ---------------- shared helpers ---------------- */

/* Push `bottleneck` along the s-t path recorded in from_arc[]
 * (from_arc[v] = the arc entering v on the path). */
static igraph_real_t augment_along_path(flow_t *f, igraph_integer_t s,
                                        igraph_integer_t t,
                                        const igraph_integer_t *from_arc)
{
    igraph_real_t bottle = IGRAPH_INFINITY;
    for (igraph_integer_t v = t; v != s; v = f->to[from_arc[v] ^ 1])
        if (f->cap[from_arc[v]] < bottle) bottle = f->cap[from_arc[v]];

    for (igraph_integer_t v = t; v != s; v = f->to[from_arc[v] ^ 1]) {
        f->cap[from_arc[v]] -= bottle;
        f->cap[from_arc[v] ^ 1] += bottle;
    }
    return bottle;
}

static void fill_edge_flows(const flow_t *f, igraph_real_t *flow)
{
    if (!flow) return;
    for (igraph_integer_t k = 0; k < f->m; k++)
        flow[k] = f->cap[2 * k + 1];   /* reverse residual == forward flow */
}

/* ---------------- Ford-Fulkerson (DFS augmenting paths) ---------------- */

igraph_real_t flow_ford_fulkerson(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                                  igraph_real_t *flow)
{
    igraph_integer_t n = f->n;
    igraph_integer_t *from_arc = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *stack = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_real_t total = 0.0;

    for (;;) {
        for (igraph_integer_t v = 0; v < n; v++) from_arc[v] = -1;
        from_arc[s] = -2;                       /* sentinel: s has no entry */

        igraph_integer_t sp = 0;
        stack[sp++] = s;
        igraph_integer_t found = 0;
        while (sp > 0 && !found) {
            igraph_integer_t u = stack[--sp];
            for (igraph_integer_t a = f->head[u]; a != -1 && !found; a = f->next[a]) {
                if (f->cap[a] > 0.0 && from_arc[f->to[a]] == -1) {
                    from_arc[f->to[a]] = a;
                    if (f->to[a] == t) { found = 1; break; }
                    stack[sp++] = f->to[a];
                }
            }
        }
        if (!found) break;                      /* no augmenting path */
        total += augment_along_path(f, s, t, from_arc);
    }

    fill_edge_flows(f, flow);
    free(from_arc);
    free(stack);
    return total;
}

/* ---------------- Edmonds-Karp (BFS augmenting paths) ---------------- */

igraph_real_t flow_edmonds_karp(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                                igraph_real_t *flow)
{
    igraph_integer_t n = f->n;
    igraph_integer_t *from_arc = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_real_t total = 0.0;

    for (;;) {
        for (igraph_integer_t v = 0; v < n; v++) from_arc[v] = -1;
        from_arc[s] = -2;                       /* sentinel: s has no entry */

        igraph_integer_t qh = 0, qt = 0;
        q[qt++] = s;
        while (qh < qt && from_arc[t] == -1) {
            igraph_integer_t u = q[qh++];
            for (igraph_integer_t a = f->head[u]; a != -1; a = f->next[a]) {
                if (f->cap[a] > 0.0 && from_arc[f->to[a]] == -1) {
                    from_arc[f->to[a]] = a;
                    q[qt++] = f->to[a];
                }
            }
        }
        if (from_arc[t] == -1) break;           /* no augmenting path */
        total += augment_along_path(f, s, t, from_arc);
    }

    fill_edge_flows(f, flow);
    free(from_arc);
    free(q);
    return total;
}

/* ---------------- Dinic (level graph + blocking flow) ---------------- */

/* Build the level graph (BFS over residual arcs). Returns true iff t is
 * reachable, filling level[] with layers. */
static bool dinic_levels(const flow_t *f, igraph_integer_t s, igraph_integer_t t,
                         igraph_integer_t *level)
{
    igraph_integer_t n = f->n;
    for (igraph_integer_t v = 0; v < n; v++) level[v] = -1;

    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t qh = 0, qt = 0;
    q[qt++] = s;
    level[s] = 0;

    while (qh < qt) {
        igraph_integer_t u = q[qh++];
        if (u == t) continue;                   /* do not expand beyond t */
        for (igraph_integer_t a = f->head[u]; a != -1; a = f->next[a]) {
            igraph_integer_t w = f->to[a];
            if (f->cap[a] > 0.0 && level[w] == -1) {
                level[w] = level[u] + 1;
                q[qt++] = w;
            }
        }
    }
    free(q);
    return level[t] != -1;
}

/* Recursion depth is bounded by the length of a shortest s-t path in the
 * level graph (fine for the course-scale datasets). */
static igraph_real_t dinic_send(flow_t *f, const igraph_integer_t *level,
                                igraph_integer_t *iter, igraph_integer_t u,
                                igraph_integer_t t, igraph_real_t pushed)
{
    if (u == t) return pushed;
    for (; iter[u] != -1; iter[u] = f->next[iter[u]]) {
        igraph_integer_t a = iter[u];
        igraph_integer_t w = f->to[a];
        if (f->cap[a] > 0.0 && level[w] == level[u] + 1) {
            igraph_real_t d = dinic_send(f, level, iter, w, t,
                                         pushed < f->cap[a] ? pushed : f->cap[a]);
            if (d > 0.0) {
                f->cap[a] -= d;
                f->cap[a ^ 1] += d;
                return d;
            }
        }
    }
    return 0.0;
}

igraph_real_t flow_dinic(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                         igraph_real_t *flow)
{
    igraph_integer_t n = f->n;
    igraph_integer_t *level = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *iter  = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_real_t total = 0.0;

    while (dinic_levels(f, s, t, level)) {
        for (igraph_integer_t v = 0; v < n; v++) iter[v] = f->head[v];
        igraph_real_t d;
        while ((d = dinic_send(f, level, iter, s, t, IGRAPH_INFINITY)) > 0.0)
            total += d;
    }

    fill_edge_flows(f, flow);
    free(level);
    free(iter);
    return total;
}

/* ---------------- Preflow-push (Goldberg-Tarjan, FIFO) ---------------- */

igraph_real_t flow_preflow_push(flow_t *f, igraph_integer_t s, igraph_integer_t t,
                                igraph_real_t *flow)
{
    igraph_integer_t n = f->n;
    igraph_integer_t *h = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_real_t *excess = xcalloc((size_t)n, sizeof(igraph_real_t));
    bool *in_q = xcalloc((size_t)n, sizeof(bool));
    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));

    /* 1. heights: reverse BFS from t in the residual network.
     * rev_head[x] lists arcs a with to[a] == x. */
    igraph_integer_t *rev_head = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *rev_next = xmalloc((size_t)(2 * f->m) * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) rev_head[v] = -1;
    for (igraph_integer_t a = 0; a < 2 * f->m; a++) {
        rev_next[a] = rev_head[f->to[a]];
        rev_head[f->to[a]] = a;
    }

    for (igraph_integer_t v = 0; v < n; v++) h[v] = -1;
    igraph_integer_t qh = 0, qt = 0;
    q[qt++] = t;
    h[t] = 0;
    while (qh < qt) {
        igraph_integer_t x = q[qh++];
        for (igraph_integer_t ra = rev_head[x]; ra != -1; ra = rev_next[ra]) {
            igraph_integer_t u = f->to[ra ^ 1]; /* origin of arc ra: u -> x */
            if (f->cap[ra] > 0.0 && h[u] == -1) {
                h[u] = h[x] + 1;
                q[qt++] = u;
            }
        }
    }
    for (igraph_integer_t v = 0; v < n; v++)
        if (h[v] == -1) h[v] = n;               /* unreachable from t */
    h[s] = n;

    /* 2. initial preflow out of s */
    qh = qt = 0;
    for (igraph_integer_t a = f->head[s]; a != -1; a = f->next[a]) {
        igraph_integer_t w = f->to[a];
        if (f->cap[a] > 0.0 && w != s) {
            excess[w] += f->cap[a];
            f->cap[a ^ 1] += f->cap[a];
            f->cap[a] = 0.0;
            if (w != t && !in_q[w]) { in_q[w] = true; q[qt++] = w; }
        }
    }

    /* 3. FIFO discharge */
    igraph_real_t total = 0.0;
    while (qh < qt) {
        igraph_integer_t u = q[qh++];
        in_q[u] = false;
        while (excess[u] > 0.0) {
            igraph_integer_t a;
            for (a = f->head[u]; a != -1; a = f->next[a])
                if (f->cap[a] > 0.0 && h[u] == h[f->to[a]] + 1)
                    break;
            if (a != -1) {                      /* admissible arc: push */
                igraph_integer_t w = f->to[a];
                igraph_real_t d = excess[u] < f->cap[a] ? excess[u] : f->cap[a];
                f->cap[a] -= d;
                f->cap[a ^ 1] += d;
                excess[u] -= d;
                excess[w] += d;
                if (w == t) total += d;
                else if (w != s && !in_q[w]) { in_q[w] = true; q[qt++] = w; }
            } else {                            /* relabel */
                igraph_real_t best = IGRAPH_INFINITY;
                for (a = f->head[u]; a != -1; a = f->next[a])
                    if (f->cap[a] > 0.0 && (igraph_real_t)h[f->to[a]] < best)
                        best = (igraph_real_t)h[f->to[a]];
                h[u] = (best == IGRAPH_INFINITY) ? n : (igraph_integer_t)best + 1;
            }
        }
    }

    fill_edge_flows(f, flow);
    free(h); free(excess); free(in_q); free(q);
    free(rev_head); free(rev_next);
    return total;
}

/* ---------------- Min cut ---------------- */

bool *flow_mincut_side(const flow_t *f, igraph_integer_t s)
{
    igraph_integer_t n = f->n;
    bool *side = xcalloc((size_t)n, sizeof(bool));
    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t qh = 0, qt = 0;
    q[qt++] = s;
    side[s] = true;
    while (qh < qt) {
        igraph_integer_t u = q[qh++];
        for (igraph_integer_t a = f->head[u]; a != -1; a = f->next[a]) {
            igraph_integer_t w = f->to[a];
            if (f->cap[a] > 0.0 && !side[w]) {
                side[w] = true;
                q[qt++] = w;
            }
        }
    }
    free(q);
    return side;
}
