#include "connectivity.h"

#include <stdio.h>
#include <stdlib.h>

#include "util.h"

static void warn_empty(void)
{
    fprintf(stderr, "connectivity: not implemented\n");
    exit(1);
}

void scc_run(const csr_t *g, scc_result_t *res) { (void)g; (void)res; warn_empty(); }
void scc_result_destroy(scc_result_t *res) { (void)res; }
void articulation_points_run(const csr_t *g, ap_result_t *res) { (void)g; (void)res; warn_empty(); }
void ap_result_destroy(ap_result_t *res) { (void)res; }
void biconnected_run(const csr_t *g, biconnected_result_t *res) { (void)g; (void)res; warn_empty(); }
void biconnected_result_destroy(biconnected_result_t *res) { (void)res; }
void bridges_run(const csr_t *g, bridges_result_t *res) { (void)g; (void)res; warn_empty(); }
void bridges_result_destroy(bridges_result_t *res) { (void)res; }
void edge2_components_run(const csr_t *g, edge2_result_t *res) { (void)g; (void)res; warn_empty(); }
void edge2_result_destroy(edge2_result_t *res) { (void)res; }
