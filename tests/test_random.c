#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "edgegraph.h"
#include "karger.h"
#include "maxcut.h"
#include "uf.h"
#include "util.h"

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

static void test_uf(void)
{
    uf_t u;
    uf_init(&u, 10);
    uf_union(&u, 0, 1);
    uf_union(&u, 1, 2);
    uf_union(&u, 3, 4);
    check("uf: 0,1,2 same", uf_find(&u, 0) == uf_find(&u, 2));
    check("uf: 3,4 same", uf_find(&u, 3) == uf_find(&u, 4));
    check("uf: groups differ", uf_find(&u, 0) != uf_find(&u, 3));
    check("uf: 5 alone", uf_find(&u, 5) == 5 && uf_find(&u, 5) != uf_find(&u, 0));
    uf_union(&u, 4, 0);
    check("uf: all joined", uf_find(&u, 3) == uf_find(&u, 1));
    uf_destroy(&u);
}

/* triangle: maxcut 2, mincut 2 */
static void test_triangle(void)
{
    igraph_integer_t edges[] = {0, 1, 1, 2, 2, 0};
    igraph_t g = make_graph(edges, 3, 3, IGRAPH_UNDIRECTED);
    edge_graph_t e;
    edge_graph_build(&e, &g);
    igraph_destroy(&g);

    rng_t rng;
    rng_seed(&rng, 99);
    bool side[3];
    igraph_integer_t best = maxcut_best(&e, &rng, 200, side);
    check("maxcut triangle: best == 2", best == 2);
    check("maxcut triangle: valid", maxcut_value(&e, side) == best);

    igraph_integer_t c = karger_mincut(&e, &rng, 200, side);
    check("karger triangle: mincut == 2", c == 2);

    check("brute triangle: mincut == 2", karger_brute_mincut(&e) == 2);
    check("stein triangle: mincut == 2", karger_stein(&e, &rng) == 2);

    edge_graph_destroy(&e);
}

/* 2x3 ladder (6 vertices, 7 edges): mincut = 2 (corner degree) */
static void test_ladder(void)
{
    igraph_integer_t edges[] = {
        0,1, 1,2,      /* top row */
        3,4, 4,5,      /* bottom row */
        0,3, 1,4, 2,5  /* rungs */
    };
    igraph_t g = make_graph(edges, 7, 6, IGRAPH_UNDIRECTED);
    edge_graph_t e;
    edge_graph_build(&e, &g);
    igraph_destroy(&g);

    check("brute ladder: mincut == 2", karger_brute_mincut(&e) == 2);

    rng_t rng;
    rng_seed(&rng, 1234);
    bool side[6];
    igraph_integer_t best = karger_mincut(&e, &rng, 500, side);
    check("karger ladder: mincut == 2 in 500 trials", best == 2);

    rng_seed(&rng, 42);
    check("stein ladder: mincut == 2", karger_stein(&e, &rng) == 2);

    edge_graph_destroy(&e);
}

/* path P4: mincut 1; maxcut is 3 */
static void test_path(void)
{
    igraph_integer_t edges[] = {0, 1, 1, 2, 2, 3};
    igraph_t g = make_graph(edges, 3, 4, IGRAPH_UNDIRECTED);
    edge_graph_t e;
    edge_graph_build(&e, &g);
    igraph_destroy(&g);

    rng_t rng;
    rng_seed(&rng, 7);
    bool side[4];
    igraph_integer_t mc = maxcut_best(&e, &rng, 100, side);
    check("maxcut path: best == 3", mc == 3);

    igraph_integer_t c = karger_mincut(&e, &rng, 100, side);
    check("karger path: mincut == 1", c == 1);
    edge_graph_destroy(&e);
}

int main(void)
{
    printf("test_random\n");
    test_uf();
    test_triangle();
    test_ladder();
    test_path();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
