#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "csr.h"
#include "search.h"

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

/* Manhattan distance on a grid of width w. */
typedef struct { igraph_integer_t w; } grid_ctx_t;

static igraph_real_t manhattan(igraph_integer_t v, igraph_integer_t target, void *ctx)
{
    igraph_integer_t w = ((grid_ctx_t *)ctx)->w;
    igraph_integer_t rv = v / w, cv = v % w;
    igraph_integer_t rt = target / w, ct = target % w;
    return (igraph_real_t)(labs(rv - rt) + labs(cv - ct));
}

static void test_bfs_path(void)
{
    /* chain 0-1-2-3 */
    igraph_integer_t edges[] = {0, 1, 1, 2, 2, 3};
    igraph_t g = make_graph(edges, 3, 4, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    bfs_result_t r;
    bfs_run(&c, 0, &r);
    check("bfs chain: 4 reached", r.n_reached == 4);
    check("bfs chain: dist[2]=2", r.dist[2] == 2);
    check("bfs chain: parent[2]=1", r.parent[2] == 1);
    check("bfs chain: order[0]=0", r.order[0] == 0);
    check("bfs chain: dist[3]=3", r.dist[3] == 3);
    bfs_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_bfs_disconnected(void)
{
    /* two components: 0-1 and 2-3 */
    igraph_integer_t edges[] = {0, 1, 2, 3};
    igraph_t g = make_graph(edges, 2, 4, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    bfs_result_t r;
    bfs_run(&c, 0, &r);
    check("bfs disconnected: reached 2", r.n_reached == 2);
    check("bfs disconnected: dist[2]=-1", r.dist[2] == -1);
    check("bfs disconnected: parent[1]=0", r.parent[1] == 0);
    bfs_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_dfs(void)
{
    /* star: 0-1, 0-2, 0-3 */
    igraph_integer_t edges[] = {0, 1, 0, 2, 0, 3};
    igraph_t g = make_graph(edges, 3, 4, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    dfs_result_t r;
    dfs_run(&c, 0, &r);
    check("dfs star: all visited", r.dis[1] != -1 && r.dis[2] != -1 && r.dis[3] != -1);
    check("dfs star: parent of leaf is 0", r.parent[1] == 0 && r.parent[2] == 0 && r.parent[3] == 0);
    check("dfs star: dis[0]=0", r.dis[0] == 0);
    check("dfs star: one component", r.comp[0] == r.comp[1] && r.comp[1] == r.comp[3]);
    int times_ok = 1;
    for (igraph_integer_t v = 0; v < c.n; v++)
        if (r.dis[v] < 0 || r.fin[v] <= r.dis[v]) times_ok = 0;
    check("dfs star: dis < fin for all", times_ok);
    dfs_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_dfs_components(void)
{
    igraph_integer_t edges[] = {0, 1, 2, 3, 3, 4};
    igraph_t g = make_graph(edges, 3, 5, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    dfs_result_t r;
    dfs_run(&c, 2, &r);
    check("dfs comps: two components",
          r.comp[0] == r.comp[1] && r.comp[2] == r.comp[3] && r.comp[3] == r.comp[4]);
    check("dfs comps: different ids", r.comp[0] != r.comp[2]);
    dfs_result_destroy(&r);

    igraph_destroy(&g);
    csr_destroy(&c);
}

static void test_astar_grid(void)
{
    /* 3x3 grid, edges between orthogonal neighbours */
    igraph_integer_t edges[] = {
        0,1, 1,2, 3,4, 4,5, 6,7, 7,8,  /* rows */
        0,3, 3,6, 1,4, 4,7, 2,5, 5,8   /* columns */
    };
    igraph_t g = make_graph(edges, 12, 9, IGRAPH_UNDIRECTED);
    csr_t c;
    csr_build(&c, &g, false);

    grid_ctx_t ctx = {3};
    igraph_real_t dist[9];
    igraph_integer_t parent[9];
    bool reached = astar_run(&c, 0, 8, manhattan, &ctx, dist, parent);
    check("astar: reaches target", reached);
    check("astar: dist[8] == 4", reached && dist[8] == 4.0);
    /* walk parent chain back to source */
    igraph_integer_t hops = 0, v = 8;
    while (v != 0 && hops < 10) { v = parent[v]; hops++; }
    check("astar: path length 4", hops == 4);
    igraph_destroy(&g);
    csr_destroy(&c);
}

int main(void)
{
    printf("test_search\n");
    test_bfs_path();
    test_bfs_disconnected();
    test_dfs();
    test_dfs_components();
    test_astar_grid();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
