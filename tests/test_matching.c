#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "csr.h"
#include "hungarian.h"
#include "matching.h"
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

/* Verify that the match arrays encode a valid matching of expected size. */
static int check_matching_valid(const csr_t *c, const bool *side,
                                const igraph_integer_t *ml,
                                const igraph_integer_t *mr,
                                igraph_integer_t expect)
{
    igraph_integer_t size = 0;
    for (igraph_integer_t v = 0; v < c->n; v++) {
        if (ml[v] != -1) {
            if (mr[ml[v]] != v) return 0;               /* consistency */
            size++;
        }
        if (mr[v] != -1 && ml[mr[v]] != v) return 0;
    }
    if (size != expect) return 0;
    /* matched pairs must be edges crossing the bipartition */
    for (igraph_integer_t v = 0; v < c->n; v++) {
        if (ml[v] == -1) continue;
        if (side[v] == side[ml[v]]) return 0;           /* same side! */
        int found = 0;
        for (igraph_integer_t a = c->offsets[v]; a < c->offsets[v + 1]; a++)
            if (c->targets[a] == ml[v]) found = 1;
        if (!found) return 0;
    }
    return 1;
}

/* Diamond: L={0,1}, R={2,3}; edges 0-2, 0-3, 1-2.  Perfect matching = 2. */
static void test_hk_diamond(void)
{
    igraph_integer_t edges[] = {0, 2, 0, 3, 1, 2};
    igraph_t g = make_graph(edges, 3, 4, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);
    bool side[] = {true, true, false, false};
    igraph_integer_t ml[4], mr[4];

    igraph_integer_t sz = matching_hopcroft_karp(&c, side, ml, mr);
    check("hk diamond: size 2", sz == 2);
    check("hk diamond: valid matching", check_matching_valid(&c, side, ml, mr, 2));
    igraph_destroy(&g);
    csr_destroy(&c);
}

/* 3+3 perfect: L={0,1,2}, R={3,4,5}; edges 0-3, 0-4, 1-3, 2-5. */
static void test_hk_perfect(void)
{
    igraph_integer_t edges[] = {0, 3, 0, 4, 1, 3, 2, 5};
    igraph_t g = make_graph(edges, 4, 6, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);
    bool side[] = {true, true, true, false, false, false};
    igraph_integer_t ml[6], mr[6];

    igraph_integer_t sz = matching_hopcroft_karp(&c, side, ml, mr);
    check("hk perfect: size 3", sz == 3);
    check("hk perfect: valid", check_matching_valid(&c, side, ml, mr, 3));
    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_hk_vs_igraph(void)
{
    igraph_integer_t edges[] = {0, 4, 0, 5, 1, 4, 1, 6, 2, 6, 2, 7, 3, 5, 3, 7};
    igraph_t g = make_graph(edges, 8, 8, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);
    bool side[] = {true, true, true, true, false, false, false, false};
    igraph_integer_t ml[8], mr[8];

    igraph_integer_t sz = matching_hopcroft_karp(&c, side, ml, mr);

    igraph_vector_bool_t types;
    igraph_vector_bool_init(&types, 8);
    for (igraph_integer_t i = 0; i < 8; i++) VECTOR(types)[i] = side[i];
    igraph_integer_t oracle_size = 0;
    igraph_real_t oracle_weight = 0.0;
    igraph_vector_int_t oracle_match;
    igraph_vector_int_init(&oracle_match, 0);
    igraph_maximum_bipartite_matching(&g, &types, &oracle_size, &oracle_weight,
                                      &oracle_match, NULL, 0.0);
    check("hk: size == igraph size", sz == oracle_size);
    check("hk: valid", check_matching_valid(&c, side, ml, mr, sz));
    igraph_vector_bool_destroy(&types);
    igraph_vector_int_destroy(&oracle_match);
    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_viaflow(void)
{
    igraph_integer_t edges[] = {0, 2, 0, 3, 1, 2};
    igraph_t g = make_graph(edges, 3, 4, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);
    bool side[] = {true, true, false, false};
    igraph_integer_t ml[4], mr[4];

    igraph_integer_t sz = matching_via_flow(&c, side, ml, mr);
    check("viaflow diamond: size 2", sz == 2);
    check("viaflow diamond: valid", check_matching_valid(&c, side, ml, mr, 2));
    igraph_destroy(&g);
    csr_destroy(&c);
}

static void permute(igraph_integer_t *p, igraph_integer_t k, igraph_integer_t n,
                    const igraph_real_t *c, igraph_real_t *best)
{
    if (k == n) {
        igraph_real_t s = 0.0;
        for (igraph_integer_t i = 0; i < n; i++) s += c[i * n + p[i]];
        if (s < *best) *best = s;
        return;
    }
    for (igraph_integer_t i = k; i < n; i++) {
        igraph_integer_t tmp = p[k]; p[k] = p[i]; p[i] = tmp;
        permute(p, k + 1, n, c, best);
        tmp = p[k]; p[k] = p[i]; p[i] = tmp;
    }
}

static void test_hungarian_small(void)
{
    /* [[1,2],[3,4]]: optimum = 5 */
    igraph_real_t c1[] = {1, 2, 3, 4};
    igraph_integer_t a1[2];
    igraph_real_t v1 = hungarian_solve(c1, 2, a1);
    check("hungarian 2x2 == 5", v1 == 5.0);
    check("hungarian 2x2 assignment valid",
          a1[0] >= 0 && a1[1] >= 0 && a1[0] != a1[1]);

    /* [[4,1],[2,3]]: row0->col1 (1), row1->col0 (2) = 3 */
    igraph_real_t c2[] = {4, 1, 2, 3};
    igraph_integer_t a2[2];
    igraph_real_t v2 = hungarian_solve(c2, 2, a2);
    check("hungarian [[4,1],[2,3]] == 3", v2 == 3.0);
    check("hungarian column assignment", (a2[0] == 1 && a2[1] == 0));
}

static void test_hungarian_vs_brute(void)
{
    const igraph_integer_t n = 4;
    igraph_real_t c[16];
    rng_t r;
    rng_seed(&r, 12345);
    for (igraph_integer_t i = 0; i < n * n; i++)
        c[i] = (igraph_real_t)(int)(rng_uniform(&r) * 20.0);

    igraph_integer_t a[4];
    igraph_real_t got = hungarian_solve(c, n, a);

    igraph_integer_t perm[4] = {0, 1, 2, 3};
    igraph_real_t best = 1e300;
    permute(perm, 0, n, c, &best);
    check("hungarian 4x4 == brute force", got == best);
}

int main(void)
{
    printf("test_matching\n");
    test_hk_diamond();
    test_hk_perfect();
    test_hk_vs_igraph();
    test_viaflow();
    test_hungarian_small();
    test_hungarian_vs_brute();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
