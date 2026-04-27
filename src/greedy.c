#include "greedy.h"
#include <stdlib.h>
#include <stdbool.h>

igraph_integer_t greedy_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    igraph_integer_t max_color = 0;

    for (igraph_integer_t v = 0; v < n; v++) {
        igraph_neighbors(g, &neighbors, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false);
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        igraph_integer_t limit = deg + 1;
        bool *used = calloc((size_t)limit, sizeof(bool));

        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neighbors)[i];
            if (color[w] >= 0 && color[w] < limit) {
                used[color[w]] = true;
            }
        }

        igraph_integer_t c = 0;
        while (used[c]) c++;
        color[v] = c;
        if (c > max_color) max_color = c;
        free(used);
    }

    igraph_vector_int_destroy(&neighbors);
    return max_color + 1;
}
