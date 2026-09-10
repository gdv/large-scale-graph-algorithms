#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csr.h"
#include "util.h"

static int passed = 0;
static int failed = 0;

static void check(const char *name, int ok)
{
    if (ok) { passed++; printf("  PASS %s\n", name); }
    else    { failed++; printf("  FAIL %s\n", name); }
}

/* Helper: build an igraph from a flat (from,to) edge array. */
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

static void test_rng(void)
{
    rng_t a, b;
    rng_seed(&a, 42);
    rng_seed(&b, 42);
    int same = 1;
    for (int i = 0; i < 100; i++)
        if (rng_next(&a) != rng_next(&b)) same = 0;
    check("rng deterministic with same seed", same);

    rng_seed(&a, 7);
    int in_range = 1;
    for (int i = 0; i < 1000; i++) {
        double u = rng_uniform(&a);
        if (!(u >= 0.0 && u < 1.0)) in_range = 0;
        if (rng_choice(&a, 10) >= 10) in_range = 0;
    }
    check("rng uniform in [0,1) and choice in [0,n)", in_range);
}

static void test_csr_directed(void)
{
    /* 0 -> 1, 1 -> 2 */
    igraph_integer_t edges[] = {0, 1, 1, 2};
    igraph_t g = make_graph(edges, 2, 3, IGRAPH_DIRECTED);
    csr_t c;
    csr_build(&c, &g, true);

    check("directed csr: 2 arcs", c.m == 2);
    check("directed csr: n=3", c.n == 3);
    check("directed csr: rev is NULL", c.rev == NULL);
    check("directed csr: deg(0)=1", csr_degree(&c, 0) == 1);
    check("directed csr: deg(2)=0", csr_degree(&c, 2) == 0);
    check("directed csr: arc 0 -> 1", c.targets[c.offsets[0]] == 1);
    check("directed csr: arc 1 -> 2", c.targets[c.offsets[1]] == 2);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_csr_undirected(void)
{
    /* triangle 0-1-2, plus dangling edge 3-4 */
    igraph_integer_t edges[] = {0, 1, 1, 2, 2, 0, 3, 4};
    igraph_t g = make_graph(edges, 4, 5, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    check("undirected csr: 8 arcs", c.m == 8);
    check("undirected csr: rev not NULL", c.rev != NULL);
    check("undirected csr: deg(0)=2", csr_degree(&c, 0) == 2);
    check("undirected csr: deg(3)=1", csr_degree(&c, 3) == 1);
    check("undirected csr: deg(4)=1", csr_degree(&c, 4) == 1);

    /* every arc's reverse points back at it */
    int pairs_ok = 1;
    for (igraph_integer_t a = 0; a < c.m; a++)
        if (c.rev[c.rev[a]] != a) pairs_ok = 0;
    check("undirected csr: rev is an involution", pairs_ok);

    /* every (u -> w) arc has a sibling (w -> u) */
    int sibling_ok = 1;
    for (igraph_integer_t u = 0; u < c.n; u++)
        for (igraph_integer_t i = c.offsets[u]; i < c.offsets[u + 1]; i++) {
            igraph_integer_t w = c.targets[i];
            igraph_integer_t b = c.rev[i];
            if (b < 0 || b >= c.m) { sibling_ok = 0; continue; }
            if (c.offsets[w] > b || b >= c.offsets[w + 1]) sibling_ok = 0;
            if (c.targets[b] != u) sibling_ok = 0;
        }
    check("undirected csr: sibling arcs are reciprocal", sibling_ok);

    igraph_destroy(&g);
    csr_destroy(&c);
}

int main(void)
{
    printf("test_csr\n");
    test_rng();
    test_csr_directed();
    test_csr_undirected();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
