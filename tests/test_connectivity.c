#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "csr.h"
#include "connectivity.h"

static int passed = 0;
static int failed = 0;

static void check(const char *name, int ok)
{
    if (ok) { passed++; printf("  PASS %s\n", name); }
    else    { failed++; printf("  FAIL %s\n", name); }
}

static igraph_t make_graph(const igraph_integer_t *edges, igraph_integer_t m,
                           igraph_integer_t n, igraph_bool_t directed)
{
    igraph_t g;
    igraph_vector_int_t elist;
    igraph_vector_int_init(&elist, 2 * m);
    for (igraph_integer_t i = 0; i < m; i++) {
        VECTOR(elist)[2 * i]     = edges[2 * i];
        VECTOR(elist)[2 * i + 1] = edges[2 * i + 1];
    }
    igraph_create(&g, &elist, n, directed);
    igraph_vector_int_destroy(&elist);
    return g;
}

/* Graph used by this whole suite (undirected "bow-tie"):
 *   0-1, 0-2, 1-2  (triangle),  0-3  (dangling edge),  plus edge 4-5.
 *
 * articulation points: {0}
 * bridges: {0-3, 4-5}
 * biconnected blocks: {0-1,0-2,1-2} and {0-3} and {4-5}  -> 3 blocks
 * 2-edge comps: {0,1,2}, {3}, {4}, {5}  -> 4 components
 */
static igraph_integer_t g_edges[] = {0,1, 0,2, 1,2, 0,3, 4,5};
static const igraph_integer_t g_m = 5, g_n = 6;

static void test_scc_basic(void)
{
    /* digraph: 0->1->2->0 (one SCC), 3->1, and 4 isolated */
    igraph_integer_t edges[] = {0,1, 1,2, 2,0, 3,1};
    igraph_t g = make_graph(edges, 4, 5, IGRAPH_DIRECTED);
    csr_t c;
    csr_build(&c, &g, true);

    scc_result_t r;
    scc_run(&c, &r);
    check("scc: 3 components", r.n_comp == 3);
    check("scc: 0,1,2 in one component", r.comp[0] == r.comp[1] && r.comp[1] == r.comp[2]);
    check("scc: 3 and 4 are singletons",
          r.comp[3] != r.comp[0] && r.comp[4] != r.comp[0] && r.comp[3] != r.comp[4]);
    scc_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_scc_vs_igraph(void)
{
    igraph_integer_t edges[] = {0,1, 1,0, 1,2, 2,3, 3,2, 4,5, 5,6, 6,4, 7,0};
    igraph_t g = make_graph(edges, 9, 8, IGRAPH_DIRECTED);
    csr_t c;
    csr_build(&c, &g, true);

    scc_result_t r;
    scc_run(&c, &r);

    igraph_vector_int_t membership;
    igraph_vector_int_init(&membership, 0);
    igraph_integer_t n_igraph = 0;
    igraph_connected_components(&g, &membership, NULL, &n_igraph, IGRAPH_STRONG);
    check("scc: same #components as igraph", r.n_comp == n_igraph);
    igraph_vector_int_destroy(&membership);
    scc_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_articulation(void)
{
    igraph_t g = make_graph(g_edges, g_m, g_n, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    ap_result_t r;
    articulation_points_run(&c, &r);
    check("ap: {0}", r.is_ap[0] && !r.is_ap[1] && !r.is_ap[2] && !r.is_ap[3] &&
          !r.is_ap[4] && !r.is_ap[5]);

    igraph_vector_int_t res;
    igraph_vector_int_init(&res, 0);
    igraph_articulation_points(&g, &res);
    check("ap: matches igraph count",
          (igraph_integer_t)igraph_vector_int_size(&res) == (r.is_ap[0] ? 1 : 0));
    igraph_vector_int_destroy(&res);
    ap_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_bridges(void)
{
    igraph_t g = make_graph(g_edges, g_m, g_n, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    bridges_result_t r;
    bridges_run(&c, &r);
    igraph_integer_t n_bridge_edges = 0;
    for (igraph_integer_t a = 0; a < c.m; a++)
        if (r.is_bridge[a] && a < c.rev[a]) n_bridge_edges++;
    check("bridges: exactly 2 bridge edges", n_bridge_edges == 2);

    igraph_vector_int_t res;
    igraph_vector_int_init(&res, 0);
    igraph_bridges(&g, &res);
    check("bridges: matches igraph count",
          (igraph_integer_t)igraph_vector_int_size(&res) == 2);
    igraph_vector_int_destroy(&res);
    bridges_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_biconnected(void)
{
    igraph_t g = make_graph(g_edges, g_m, g_n, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    biconnected_result_t r;
    biconnected_run(&c, &r);
    check("biconnected: 3 blocks", r.n_blocks == 3);
    igraph_integer_t tri = 0, single = 0;
    for (igraph_integer_t b = 0; b < r.n_blocks; b++) {
        if (r.block_edges[b] == 3) tri++;
        if (r.block_edges[b] == 1) single++;
    }
    check("biconnected: 1 triangle block + 2 trivial", tri == 1 && single == 2);

    igraph_int_t no = 0;
    igraph_vector_int_list_t comps, edges_l;
    igraph_vector_int_t aps;
    igraph_vector_int_list_init(&comps, 0);
    igraph_vector_int_list_init(&edges_l, 0);
    igraph_vector_int_init(&aps, 0);
    igraph_biconnected_components(&g, &no, NULL, &edges_l, &comps, &aps);
    check("biconnected: matches igraph #components",
          (igraph_integer_t)no == r.n_blocks);
    igraph_vector_int_list_destroy(&comps);
    igraph_vector_int_list_destroy(&edges_l);
    igraph_vector_int_destroy(&aps);
    biconnected_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_edge2(void)
{
    igraph_t g = make_graph(g_edges, g_m, g_n, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    edge2_result_t r;
    edge2_components_run(&c, &r);
    check("2-edge: 4 components", r.n_comp == 4);
    check("2-edge: triangle coherent", r.comp[0] == r.comp[1] && r.comp[1] == r.comp[2]);
    check("2-edge: 3,4,5 isolated",
          r.comp[3] != r.comp[0] && r.comp[4] != r.comp[0] && r.comp[5] != r.comp[0]);
    edge2_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

int main(void)
{
    printf("test_connectivity\n");
    test_scc_basic();
    test_scc_vs_igraph();
    test_articulation();
    test_bridges();
    test_biconnected();
    test_edge2();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
