#include "connectivity.h"

#include <stdlib.h>

#include "util.h"

/* ---------------- SCC (Tarjan, iterative) ---------------- */

void scc_run(const csr_t *g, scc_result_t *res)
{
    igraph_integer_t n = g->n;
    igraph_integer_t *idx  = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *low  = xmalloc((size_t)n * sizeof(igraph_integer_t));
    bool *onstack = xcalloc((size_t)n, sizeof(bool));
    res->comp = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) idx[v] = 0;

    typedef struct {
        igraph_integer_t v, parent, next_arc;
    } conn_frame_t;

    conn_frame_t *frames = xmalloc((size_t)n * sizeof(conn_frame_t));
    igraph_integer_t *stk = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t sp = 0, tp = 0, counter = 1, n_comp = 0;

    for (igraph_integer_t s = 0; s < n; s++) {
        if (idx[s] != 0) continue;
        idx[s] = low[s] = counter++;
        stk[tp++] = s;
        onstack[s] = true;
        frames[sp++] = (conn_frame_t){s, -1, g->offsets[s]};

        while (sp > 0) {
            conn_frame_t *fr = &frames[sp - 1];
            igraph_integer_t u = fr->v;
            if (fr->next_arc < g->offsets[u + 1]) {
                igraph_integer_t a = fr->next_arc++;
                igraph_integer_t w = g->targets[a];
                if (idx[w] == 0) {                 /* tree edge */
                    idx[w] = low[w] = counter++;
                    stk[tp++] = w;
                    onstack[w] = true;
                    frames[sp++] = (conn_frame_t){w, u, g->offsets[w]};
                } else if (onstack[w]) {           /* edge to a stacked vertex */
                    if (idx[w] < low[u]) low[u] = idx[w];
                }
            } else {                               /* exit u */
                if (fr->parent != -1 && low[u] < low[fr->parent])
                    low[fr->parent] = low[u];
                if (low[u] == idx[u]) {            /* u roots an SCC */
                    igraph_integer_t w;
                    do {
                        w = stk[--tp];
                        onstack[w] = false;
                        res->comp[w] = n_comp;
                    } while (w != u);
                    n_comp++;
                }
                sp--;
            }
        }
    }
    res->n_comp = n_comp;

    xfree(idx); xfree(low); xfree(onstack); xfree(frames); xfree(stk);
}

void scc_result_destroy(scc_result_t *res)
{
    xfree(res->comp);
    res->comp = NULL;
}

/* ---------------- Iterative lowpoint DFS machinery ----------------
 * One frame stack powers articulation points, biconnected components
 * and bridges: frame = (vertex, DFS-tree parent, next arc to scan,
 * arc used to discover the vertex). */

typedef struct {
    igraph_integer_t v, parent, next_arc, tree_arc;
} conn2_frame_t;

/* ---------------- Articulation points ---------------- */

void articulation_points_run(const csr_t *g, ap_result_t *res)
{
    igraph_integer_t n = g->n;
    igraph_integer_t *depth = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *low   = xmalloc((size_t)n * sizeof(igraph_integer_t));
    res->is_ap = xcalloc((size_t)n, sizeof(bool));
    for (igraph_integer_t v = 0; v < n; v++) depth[v] = -1;

    conn2_frame_t *frames = xmalloc((size_t)n * sizeof(conn2_frame_t));

    for (igraph_integer_t s = 0; s < n; s++) {
        if (depth[s] != -1) continue;
        igraph_integer_t sp = 0;
        frames[sp++] = (conn2_frame_t){s, -1, g->offsets[s], -1};
        depth[s] = 0;
        low[s] = 0;
        igraph_integer_t root_children = 0;

        while (sp > 0) {
            conn2_frame_t *fr = &frames[sp - 1];
            igraph_integer_t u = fr->v;
            if (fr->next_arc < g->offsets[u + 1]) {
                igraph_integer_t a = fr->next_arc++;
                igraph_integer_t w = g->targets[a];
                if (fr->parent != -1 && a == g->rev[fr->tree_arc]) continue;
                if (depth[w] == -1) {              /* tree edge */
                    depth[w] = depth[u] + 1;
                    low[w] = depth[w];
                    if (fr->parent == -1) root_children++;
                    frames[sp++] = (conn2_frame_t){w, u, g->offsets[w], a};
                } else if (depth[w] < low[u]) {    /* back edge to ancestor */
                    low[u] = depth[w];
                }
            } else {                               /* exit u */
                if (fr->parent != -1) {
                    if (low[u] < low[fr->parent]) low[fr->parent] = low[u];
                    /* u is a tree child of fr->parent: articulation test */
                    if (low[u] >= depth[fr->parent]) res->is_ap[fr->parent] = true;
                }
                sp--;
            }
        }
        /* root rule: articulation iff it has >= 2 DFS-tree children */
        res->is_ap[s] = (root_children >= 2);
    }

    xfree(depth); xfree(low); xfree(frames);
}

void ap_result_destroy(ap_result_t *res)
{
    xfree(res->is_ap);
    res->is_ap = NULL;
}

/* ---------------- Biconnected components (edge blocks) ---------------- */

void biconnected_run(const csr_t *g, biconnected_result_t *res)
{
    igraph_integer_t n = g->n, m = g->m;
    igraph_integer_t *depth = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *low   = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) depth[v] = -1;

    res->block = xmalloc((size_t)m * sizeof(igraph_integer_t));
    for (igraph_integer_t a = 0; a < m; a++) res->block[a] = -1;

    conn2_frame_t *frames = xmalloc((size_t)n * sizeof(conn2_frame_t));
    igraph_integer_t *edge_stack = xmalloc((size_t)m * sizeof(igraph_integer_t));

    igraph_integer_t n_blocks = 0;
    igraph_integer_t *block_edges = NULL;
    igraph_integer_t n_blocks_cap = 0;

    for (igraph_integer_t s = 0; s < n; s++) {
        if (depth[s] != -1) continue;
        igraph_integer_t sp = 0, ep = 0;
        frames[sp++] = (conn2_frame_t){s, -1, g->offsets[s], -1};
        depth[s] = 0;
        low[s] = 0;

        while (sp > 0) {
            conn2_frame_t *fr = &frames[sp - 1];
            igraph_integer_t u = fr->v;
            if (fr->next_arc < g->offsets[u + 1]) {
                igraph_integer_t a = fr->next_arc++;
                igraph_integer_t w = g->targets[a];
                if (fr->parent != -1 && a == g->rev[fr->tree_arc]) continue;
                /* ^ skip only the tree arc's sibling: parallel edges to the
                 * parent then behave as back edges */
                if (depth[w] == -1) {
                    edge_stack[ep++] = a;          /* tree arc joins a block */
                    depth[w] = depth[u] + 1;
                    low[w] = depth[w];
                    frames[sp++] = (conn2_frame_t){w, u, g->offsets[w], a};
                } else if (depth[w] < depth[u]) {
                    edge_stack[ep++] = a;          /* back arc to ancestor */
                    if (depth[w] < low[u]) low[u] = depth[w];
                }
            } else {                               /* exit u */
                if (fr->parent != -1) {
                    if (low[u] < low[fr->parent]) low[fr->parent] = low[u];
                    if (low[u] >= depth[fr->parent]) {
                        /* pop block: arcs down to and including fr->tree_arc */
                        if (n_blocks == n_blocks_cap) {
                            n_blocks_cap = n_blocks_cap ? 2 * n_blocks_cap : 8;
                            block_edges = xrealloc(block_edges,
                                (size_t)n_blocks_cap * sizeof(igraph_integer_t));
                        }
                        igraph_integer_t edges = 0;
                        igraph_integer_t a;
                        do {
                            a = edge_stack[--ep];
                            res->block[a] = n_blocks;
                            res->block[g->rev[a]] = n_blocks;
                            edges++;   /* each edge is pushed exactly once */
                        } while (a != fr->tree_arc);
                        block_edges[n_blocks++] = edges;
                    }
                }
                sp--;
            }
        }
    }
    res->n_blocks = n_blocks;
    res->block_edges = block_edges;

    xfree(depth); xfree(low); xfree(frames); xfree(edge_stack);
}

void biconnected_result_destroy(biconnected_result_t *res)
{
    xfree(res->block);
    xfree(res->block_edges);
    res->block = NULL;
    res->block_edges = NULL;
}

/* ---------------- Bridges ---------------- */

void bridges_run(const csr_t *g, bridges_result_t *res)
{
    igraph_integer_t n = g->n;
    igraph_integer_t *depth = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *low   = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) depth[v] = -1;

    res->is_bridge = xcalloc((size_t)g->m, sizeof(bool));

    conn2_frame_t *frames = xmalloc((size_t)n * sizeof(conn2_frame_t));

    for (igraph_integer_t s = 0; s < n; s++) {
        if (depth[s] != -1) continue;
        igraph_integer_t sp = 0;
        frames[sp++] = (conn2_frame_t){s, -1, g->offsets[s], -1};
        depth[s] = 0;
        low[s] = 0;

        while (sp > 0) {
            conn2_frame_t *fr = &frames[sp - 1];
            igraph_integer_t u = fr->v;
            if (fr->next_arc < g->offsets[u + 1]) {
                igraph_integer_t a = fr->next_arc++;
                igraph_integer_t w = g->targets[a];
                if (fr->parent != -1 && a == g->rev[fr->tree_arc]) continue;
                /* ^ skip only the tree arc's sibling: parallel edges to the
                 * parent then behave as back edges */
                if (depth[w] == -1) {
                    depth[w] = depth[u] + 1;
                    low[w] = depth[w];
                    frames[sp++] = (conn2_frame_t){w, u, g->offsets[w], a};
                } else if (depth[w] < low[u]) {
                    low[u] = depth[w];
                }
            } else {                               /* exit u */
                if (fr->parent != -1) {
                    if (low[u] < low[fr->parent]) low[fr->parent] = low[u];
                    /* tree child u: bridge iff no back edge from u's
                     * subtree reaches fr->parent or above */
                    if (low[u] > depth[fr->parent]) {
                        res->is_bridge[fr->tree_arc] = true;
                        res->is_bridge[g->rev[fr->tree_arc]] = true;
                    }
                }
                sp--;
            }
        }
    }

    xfree(depth); xfree(low); xfree(frames);
}

void bridges_result_destroy(bridges_result_t *res)
{
    xfree(res->is_bridge);
    res->is_bridge = NULL;
}

/* ---------------- 2-edge-connected components ---------------- */

void edge2_components_run(const csr_t *g, edge2_result_t *res)
{
    igraph_integer_t n = g->n;
    res->comp = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) res->comp[v] = -1;

    bridges_result_t br;
    bridges_run(g, &br);

    igraph_integer_t *q = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t comp = 0;

    for (igraph_integer_t s = 0; s < n; s++) {
        if (res->comp[s] != -1) continue;
        igraph_integer_t qh = 0, qt = 0;
        q[qt++] = s;
        res->comp[s] = comp;
        while (qh < qt) {
            igraph_integer_t u = q[qh++];
            for (igraph_integer_t a = g->offsets[u]; a < g->offsets[u + 1]; a++) {
                if (br.is_bridge[a]) continue;     /* bridges separate comps */
                igraph_integer_t w = g->targets[a];
                if (res->comp[w] == -1) {
                    res->comp[w] = comp;
                    q[qt++] = w;
                }
            }
        }
        comp++;
    }
    res->n_comp = comp;

    bridges_result_destroy(&br);
    xfree(q);
}

void edge2_result_destroy(edge2_result_t *res)
{
    xfree(res->comp);
    res->comp = NULL;
}
