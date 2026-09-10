#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "flow.h"
#include "util.h"

static int passed = 0;
static int failed = 0;

static void check(const char *name, int ok)
{
    if (ok) { passed++; printf("  PASS %s\n", name); }
    else    { failed++; printf("  FAIL %s\n", name); }
}

/* Helper: build an igraph with a "capacity" edge attribute from flat
 * (from,to,capacity) triples. */
static igraph_t make_cap_graph(const igraph_real_t *edges, igraph_integer_t m,
                               igraph_integer_t n)
{
    igraph_t g;
    igraph_vector_int_t elist;
    igraph_vector_int_init(&elist, 2 * m);
    igraph_vector_t caps;
    igraph_vector_init(&caps, m);
    for (igraph_integer_t i = 0; i < m; i++) {
        VECTOR(elist)[2 * i]     = (igraph_integer_t)edges[3 * i];
        VECTOR(elist)[2 * i + 1] = (igraph_integer_t)edges[3 * i + 1];
        VECTOR(caps)[i]          = edges[3 * i + 2];
    }
    igraph_create(&g, &elist, n, IGRAPH_DIRECTED);
    igraph_vector_int_destroy(&elist);

    igraph_set_attribute_table(&igraph_cattribute_table);
    igraph_cattribute_EAN_setv(&g, "capacity", &caps);
    igraph_vector_destroy(&caps);
    return g;
}

/* Classic example; maxflow(0,3) = 4.
 *   0 -> 1 (3), 0 -> 2 (2), 1 -> 2 (1), 1 -> 3 (2), 2 -> 3 (3) */
static const igraph_real_t ex1[] = {0,1,3, 0,2,2, 1,2,1, 1,3,2, 2,3,3};
static const igraph_integer_t ex1_n = 4, ex1_m = 5;

/* Bipartite-like network; maxflow(0,3) = 2. */
static const igraph_real_t ex2[] = {0,1,1, 0,2,1, 1,2,1, 1,3,1, 2,3,1};
static const igraph_integer_t ex2_n = 4, ex2_m = 5;

/* No path from 0 to 3. */
static const igraph_real_t ex3[] = {0,1,5, 2,3,5};
static const igraph_integer_t ex3_n = 4, ex3_m = 2;

typedef igraph_real_t (*flow_algo_t)(flow_t *, igraph_integer_t, igraph_integer_t,
                                     igraph_real_t *);

static igraph_real_t flow_value_via(flow_algo_t run, const igraph_real_t *edges,
                                    igraph_integer_t m, igraph_integer_t n,
                                    igraph_integer_t s, igraph_integer_t t)
{
    flow_t f;
    flow_init(&f, n, m);
    for (igraph_integer_t i = 0; i < m; i++)
        flow_add_edge(&f, (igraph_integer_t)edges[3 * i],
                      (igraph_integer_t)edges[3 * i + 1], edges[3 * i + 2]);
    igraph_real_t val = run(&f, s, t, NULL);
    flow_destroy(&f);
    return val;
}

static void test_values(void)
{
    check("ff ex1 == 4",
          flow_value_via(flow_ford_fulkerson, ex1, ex1_m, ex1_n, 0, 3) == 4.0);
    check("ek ex1 == 4",
          flow_value_via(flow_edmonds_karp, ex1, ex1_m, ex1_n, 0, 3) == 4.0);
    check("dinic ex1 == 4",
          flow_value_via(flow_dinic, ex1, ex1_m, ex1_n, 0, 3) == 4.0);
    check("push ex1 == 4",
          flow_value_via(flow_preflow_push, ex1, ex1_m, ex1_n, 0, 3) == 4.0);

    check("ff ex2 == 2",
          flow_value_via(flow_ford_fulkerson, ex2, ex2_m, ex2_n, 0, 3) == 2.0);
    check("ek ex2 == 2",
          flow_value_via(flow_edmonds_karp, ex2, ex2_m, ex2_n, 0, 3) == 2.0);
    check("dinic ex2 == 2",
          flow_value_via(flow_dinic, ex2, ex2_m, ex2_n, 0, 3) == 2.0);
    check("push ex2 == 2",
          flow_value_via(flow_preflow_push, ex2, ex2_m, ex2_n, 0, 3) == 2.0);

    check("ff ex3 == 0",
          flow_value_via(flow_ford_fulkerson, ex3, ex3_m, ex3_n, 0, 3) == 0.0);
    check("ek ex3 == 0",
          flow_value_via(flow_edmonds_karp, ex3, ex3_m, ex3_n, 0, 3) == 0.0);
    check("dinic ex3 == 0",
          flow_value_via(flow_dinic, ex3, ex3_m, ex3_n, 0, 3) == 0.0);
    check("push ex3 == 0",
          flow_value_via(flow_preflow_push, ex3, ex3_m, ex3_n, 0, 3) == 0.0);
}

static void test_vs_igraph(void)
{
    igraph_t g = make_cap_graph(ex1, ex1_m, ex1_n);
    igraph_vector_t caps;
    igraph_vector_init(&caps, 0);
    igraph_cattribute_EANV(&g, "capacity", igraph_ess_all(IGRAPH_EDGEORDER_ID), &caps);

    igraph_real_t oracle = 0.0;
    igraph_maxflow_value(&g, &oracle, 0, 3, &caps, NULL);
    igraph_vector_destroy(&caps);
    igraph_destroy(&g);

    check("oracle ex1 == 4", oracle == 4.0);
    check("ff ex1 == oracle",
          flow_value_via(flow_ford_fulkerson, ex1, ex1_m, ex1_n, 0, 3) == oracle);
    check("ek ex1 == oracle",
          flow_value_via(flow_edmonds_karp, ex1, ex1_m, ex1_n, 0, 3) == oracle);
    check("dinic ex1 == oracle",
          flow_value_via(flow_dinic, ex1, ex1_m, ex1_n, 0, 3) == oracle);
    check("push ex1 == oracle",
          flow_value_via(flow_preflow_push, ex1, ex1_m, ex1_n, 0, 3) == oracle);
}

static void test_mincut(void)
{
    flow_t f;
    flow_init(&f, ex1_n, ex1_m);
    for (igraph_integer_t i = 0; i < ex1_m; i++)
        flow_add_edge(&f, (igraph_integer_t)ex1[3 * i],
                      (igraph_integer_t)ex1[3 * i + 1], ex1[3 * i + 2]);
    igraph_real_t val = flow_dinic(&f, 0, 3, NULL);

    bool *side = flow_mincut_side(&f, 0);

    /* exact invariant: sum of saturated forward arcs crossing the side
     * equals the max-flow value */
    igraph_real_t cut_cap = 0.0;
    for (igraph_integer_t u = 0; u < f.n; u++) {
        if (!side[u]) continue;
        for (igraph_integer_t a = f.head[u]; a != -1; a = f.next[a])
            if (!side[f.to[a]] && f.cap[a] == 0.0)
                cut_cap += f.cap[a ^ 1];
    }

    check("mincut: s side non-empty", side[0]);
    check("mincut: t not in s side", !side[3]);
    check("mincut: cut capacity == maxflow", cut_cap == val);
    check("maxflow value reported as 4", val == 4.0);
    free(side);
    flow_destroy(&f);
}

int main(void)
{
    printf("test_flow\n");
    test_values();
    test_vs_igraph();
    test_mincut();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
