#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include "csr.h"

#include <stdbool.h>

/* ---------------- SCC (Tarjan) ---------------- */
typedef struct {
    igraph_integer_t *comp;     /* size n, component id in [0, n_comp) */
    igraph_integer_t  n_comp;
} scc_result_t;

void scc_run(const csr_t *g, scc_result_t *res);
void scc_result_destroy(scc_result_t *res);

/* ---------------- Articulation points ---------------- */
typedef struct {
    bool *is_ap;                /* size n */
} ap_result_t;

void articulation_points_run(const csr_t *g, ap_result_t *res);
void ap_result_destroy(ap_result_t *res);

/* ---------------- Biconnected components (edge blocks) ---------------- */
typedef struct {
    igraph_integer_t n_blocks;
    igraph_integer_t *block;        /* size m (arcs), block id or -1 */
    igraph_integer_t *block_edges;  /* size n_blocks, #edges per block */
} biconnected_result_t;

void biconnected_run(const csr_t *g, biconnected_result_t *res);
void biconnected_result_destroy(biconnected_result_t *res);

/* ---------------- Bridges ---------------- */
typedef struct {
    bool *is_bridge;            /* size m (arcs); both sibling arcs flagged */
} bridges_result_t;

void bridges_run(const csr_t *g, bridges_result_t *res);
void bridges_result_destroy(bridges_result_t *res);

/* ---------------- 2-edge-connected components ---------------- */
typedef struct {
    igraph_integer_t *comp;     /* size n */
    igraph_integer_t  n_comp;
} edge2_result_t;

void edge2_components_run(const csr_t *g, edge2_result_t *res);
void edge2_result_destroy(edge2_result_t *res);

#endif
