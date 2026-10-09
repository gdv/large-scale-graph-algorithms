#include "edgegraph.h"

#include "util.h"

void edge_graph_build(edge_graph_t *e, const igraph_t *graph)
{
    e->n = (igraph_integer_t)igraph_vcount(graph);
    e->m = (igraph_integer_t)igraph_ecount(graph);
    e->u = xmalloc((size_t)e->m * sizeof(igraph_integer_t));
    e->v = xmalloc((size_t)e->m * sizeof(igraph_integer_t));

    igraph_vector_int_t elist;
    igraph_vector_int_init(&elist, 0);
    igraph_get_edgelist(graph, &elist, false);
    for (igraph_integer_t i = 0; i < e->m; i++) {
        e->u[i] = VECTOR(elist)[2 * i];
        e->v[i] = VECTOR(elist)[2 * i + 1];
    }
    igraph_vector_int_destroy(&elist);
}

void edge_graph_destroy(edge_graph_t *e)
{
    xfree(e->u);
    xfree(e->v);
    e->u = NULL;
    e->v = NULL;
}
