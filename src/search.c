#include "search.h"

#include "util.h"

void bfs_run(const csr_t *g, igraph_integer_t source, bfs_result_t *res)
{
    (void)g; (void)source;
    res->parent = NULL; res->dist = NULL; res->order = NULL; res->n_reached = 0;
}

void bfs_result_destroy(bfs_result_t *res)
{
    free(res->parent); free(res->dist); free(res->order);
    res->parent = res->dist = res->order = NULL;
}

void dfs_run(const csr_t *g, igraph_integer_t source, dfs_result_t *res)
{
    (void)g; (void)source;
    res->parent = NULL; res->dis = NULL; res->fin = NULL; res->comp = NULL;
}

void dfs_result_destroy(dfs_result_t *res)
{
    free(res->parent); free(res->dis); free(res->fin); free(res->comp);
    res->parent = res->dis = res->fin = res->comp = NULL;
}

bool astar_run(const csr_t *g, igraph_integer_t source, igraph_integer_t target,
               heuristic_t h, void *ctx,
               igraph_real_t *dist, igraph_integer_t *parent)
{
    (void)g; (void)source; (void)target; (void)h; (void)ctx;
    (void)dist; (void)parent;
    return false;
}
