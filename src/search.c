#include "search.h"

#include "util.h"

void bfs_run(const csr_t *g, igraph_integer_t source, bfs_result_t *res)
{
    igraph_integer_t n = g->n;
    res->parent = xmalloc((size_t)n * sizeof(igraph_integer_t));
    res->dist   = xmalloc((size_t)n * sizeof(igraph_integer_t));
    res->order  = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) { res->parent[v] = -1; res->dist[v] = -1; }

    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t qh = 0, qt = 0;
    q[qt++] = source;
    res->dist[source] = 0;
    res->order[0] = source;
    res->n_reached = 1;

    while (qh < qt) {
        igraph_integer_t u = q[qh++];
        for (igraph_integer_t i = g->offsets[u]; i < g->offsets[u + 1]; i++) {
            igraph_integer_t w = g->targets[i];
            if (res->dist[w] == -1) {
                res->dist[w] = res->dist[u] + 1;
                res->parent[w] = u;
                res->order[res->n_reached++] = w;
                q[qt++] = w;
            }
        }
    }
    free(q);
}

void bfs_result_destroy(bfs_result_t *res)
{
    free(res->parent); free(res->dist); free(res->order);
    res->parent = res->dist = res->order = NULL;
}

/* Iterative DFS mirroring the course slides' explicit-stack formulation:
 * each frame remembers where it was in the adjacency list, so discovery
 * and finishing times come out exactly as in the recursive version. */
typedef struct {
    igraph_integer_t v;
    igraph_integer_t next_arc;
} dfs_frame_t;

void dfs_run(const csr_t *g, igraph_integer_t source, dfs_result_t *res)
{
    igraph_integer_t n = g->n;
    res->parent = xmalloc((size_t)n * sizeof(igraph_integer_t));
    res->dis    = xmalloc((size_t)n * sizeof(igraph_integer_t));
    res->fin    = xmalloc((size_t)n * sizeof(igraph_integer_t));
    res->comp   = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) {
        res->parent[v] = -1; res->dis[v] = -1; res->fin[v] = -1; res->comp[v] = -1;
    }

    igraph_integer_t time = 0, comp = 0;
    dfs_frame_t *stack = xmalloc((size_t)n * sizeof(dfs_frame_t));

    for (igraph_integer_t s = 0; s < n; s++) {
        if (res->dis[s] != -1) continue;

        igraph_integer_t sp = 0;
        stack[sp++] = (dfs_frame_t){s, g->offsets[s]};
        res->dis[s] = time++;
        res->comp[s] = comp;

        while (sp > 0) {
            dfs_frame_t *fr = &stack[sp - 1];
            igraph_integer_t u = fr->v;
            if (fr->next_arc < g->offsets[u + 1]) {
                igraph_integer_t a = fr->next_arc++;
                igraph_integer_t w = g->targets[a];
                if (res->dis[w] == -1) {
                    res->parent[w] = u;
                    res->dis[w] = time++;
                    res->comp[w] = comp;
                    stack[sp++] = (dfs_frame_t){w, g->offsets[w]};
                }
            } else {
                res->fin[u] = time++;
                sp--;
            }
        }
        comp++;
    }
    free(stack);
}

void dfs_result_destroy(dfs_result_t *res)
{
    free(res->parent); free(res->dis); free(res->fin); free(res->comp);
    res->parent = res->dis = res->fin = res->comp = NULL;
}

/* ---- small binary min-heap on (f, vertex), used only by A* ---- */
typedef struct {
    igraph_real_t f;
    igraph_integer_t v;
} heap_item_t;

static void heap_sift_up(heap_item_t *h, igraph_integer_t i)
{
    while (i > 0) {
        igraph_integer_t p = (i - 1) / 2;
        if (h[p].f <= h[i].f) break;
        heap_item_t tmp = h[p]; h[p] = h[i]; h[i] = tmp;
        i = p;
    }
}

static void heap_sift_down(heap_item_t *h, igraph_integer_t size, igraph_integer_t i)
{
    for (;;) {
        igraph_integer_t l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < size && h[l].f < h[m].f) m = l;
        if (r < size && h[r].f < h[m].f) m = r;
        if (m == i) break;
        heap_item_t tmp = h[m]; h[m] = h[i]; h[i] = tmp;
        i = m;
    }
}

static void heap_push(heap_item_t *h, igraph_integer_t *size, igraph_real_t f, igraph_integer_t v)
{
    h[*size].f = f;
    h[*size].v = v;
    (*size)++;
    heap_sift_up(h, *size - 1);
}

static heap_item_t heap_pop(heap_item_t *h, igraph_integer_t *size)
{
    heap_item_t top = h[0];
    h[0] = h[*size - 1];
    (*size)--;
    heap_sift_down(h, *size, 0);
    return top;
}

/* A* over a unit-weight graph: pop by f = g + h, skip closed vertices
 * (lazy deletion, same trick as the Dijkstra priority queues). */
bool astar_run(const csr_t *g, igraph_integer_t source, igraph_integer_t target,
               heuristic_t h, void *ctx,
               igraph_real_t *dist, igraph_integer_t *parent)
{
    igraph_integer_t n = g->n;

    for (igraph_integer_t v = 0; v < n; v++) {
        dist[v] = IGRAPH_INFINITY;
        parent[v] = -1;
    }
    bool *closed = xcalloc((size_t)n, sizeof(bool));

    heap_item_t *heap = xmalloc((size_t)n * sizeof(heap_item_t));
    igraph_integer_t hsize = 0;

    dist[source] = 0;
    heap_push(heap, &hsize, h(source, target, ctx), source);

    bool found = false;
    while (hsize > 0) {
        heap_item_t top = heap_pop(heap, &hsize);
        igraph_integer_t u = top.v;
        if (u == target) { found = true; break; }
        if (closed[u]) continue;
        closed[u] = true;

        for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++) {
            igraph_integer_t w = g->targets[a];
            igraph_real_t nd = dist[u] + 1.0;      /* unit-weight CSR */
            if (nd < dist[w]) {
                dist[w] = nd;
                parent[w] = u;
                heap_push(heap, &hsize, nd + h(w, target, ctx), w);
            }
        }
    }

    free(closed);
    free(heap);
    return found;
}
