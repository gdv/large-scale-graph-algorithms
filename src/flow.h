#ifndef FLOW_H
#define FLOW_H

#include <igraph.h>

#include <stdbool.h>

/* Residual network: the classic sequential representation.
 *
 * Every original edge (u,v) with capacity c is stored as TWO arcs:
 *   arc 2k       : u -> v, residual capacity c     (forward)
 *   arc 2k XOR 1 : v -> u, residual capacity 0     (reverse)
 * Pushing flow along arc a decreases cap[a] and increases cap[a^1],
 * which is exactly the reverse-arc rule of the residual network.
 * Arcs are stored in adjacency lists: head[u] is the first arc leaving u,
 * next[a] the next arc in the same list.  This is the "paired arcs" trick
 * used by every textbook flow implementation.
 */
typedef struct {
    igraph_integer_t n;         /* vertices */
    igraph_integer_t m;         /* original edges; arcs = 2*m */
    igraph_integer_t e;         /* edges added so far */
    igraph_integer_t *head;     /* size n, first arc id or -1 */
    igraph_integer_t *next;     /* size 2*m */
    igraph_integer_t *to;       /* size 2*m, arc endpoint */
    igraph_real_t    *cap;      /* size 2*m, residual capacity */
} flow_t;

void flow_init(flow_t *f, igraph_integer_t n, igraph_integer_t m);
void flow_add_edge(flow_t *f, igraph_integer_t from, igraph_integer_t to,
                   igraph_real_t cap);
void flow_destroy(flow_t *f);

/* Each algorithm saturates the residual network in place and returns the
 * max-flow value. If flow != NULL it receives, per original edge k, the
 * flow sent along edge k (equals cap[2k+1] at termination). */
igraph_real_t flow_ford_fulkerson(flow_t *f, igraph_integer_t s,
                                  igraph_integer_t t, igraph_real_t *flow);
igraph_real_t flow_edmonds_karp(flow_t *f, igraph_integer_t s,
                                igraph_integer_t t, igraph_real_t *flow);
igraph_real_t flow_dinic(flow_t *f, igraph_integer_t s,
                         igraph_integer_t t, igraph_real_t *flow);
igraph_real_t flow_preflow_push(flow_t *f, igraph_integer_t s,
                                igraph_integer_t t, igraph_real_t *flow);

/* Vertices reachable from s in the final residual network (cap > 0).
 * The arcs leaving that set are exactly a minimum s-t cut. */
bool *flow_mincut_side(const flow_t *f, igraph_integer_t s);

#endif
