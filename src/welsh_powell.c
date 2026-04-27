#include "welsh_powell.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    igraph_integer_t degree;
    igraph_integer_t index;
} vertex_info;

static int cmp_degree_desc(const void *a, const void *b)
{
    const vertex_info *va = (const vertex_info *)a;
    const vertex_info *vb = (const vertex_info *)b;
    if (va->degree > vb->degree) return -1;
    if (va->degree < vb->degree) return 1;
    if (va->index < vb->index) return -1;
    return 1;
}

igraph_integer_t welsh_powell_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    vertex_info *sorted = malloc((size_t)n * sizeof(vertex_info));
    if (!sorted) { fprintf(stderr, "malloc failed\n"); exit(1); }
    for (igraph_integer_t i = 0; i < n; i++) {
        sorted[i].index = i;
        igraph_integer_t d;
        if (igraph_degree_1(g, &d, i, IGRAPH_ALL, IGRAPH_NO_LOOPS) != IGRAPH_SUCCESS) {
            fprintf(stderr, "igraph_degree_1 failed\n"); exit(1);
        }
        sorted[i].degree = d;
    }
    qsort(sorted, (size_t)n, sizeof(vertex_info), cmp_degree_desc);

    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);
    igraph_integer_t max_color = 0;

    for (igraph_integer_t idx = 0; idx < n; idx++) {
        igraph_integer_t v = sorted[idx].index;
        if (igraph_neighbors(g, &neighbors, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
            fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
        }
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        igraph_integer_t limit = deg + 1;
        bool *used = calloc((size_t)limit, sizeof(bool));
        if (!used) { fprintf(stderr, "calloc failed\n"); exit(1); }
        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neighbors)[i];
            if (color[w] >= 0 && color[w] < limit) used[color[w]] = true;
        }
        igraph_integer_t c = 0;
        while (used[c]) c++;
        color[v] = c;
        if (c > max_color) max_color = c;
        free(used);
    }
    igraph_vector_int_destroy(&neighbors);
    free(sorted);
    return max_color + 1;
}
