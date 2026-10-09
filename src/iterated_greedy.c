#include "util.h"
#include "iterated_greedy.h"
#include "greedy.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    igraph_integer_t value;
    igraph_integer_t index;
} idx_val_t;

static const igraph_vector_int_t *cmp_class_sizes_ptr = NULL;

static int cmp_class_size_desc(const void *a, const void *b)
{
    const idx_val_t *va = (const idx_val_t *)a;
    const idx_val_t *vb = (const idx_val_t *)b;
    igraph_integer_t sa = VECTOR(*cmp_class_sizes_ptr)[va->index];
    igraph_integer_t sb = VECTOR(*cmp_class_sizes_ptr)[vb->index];
    if (sa > sb) return -1;
    if (sa < sb) return 1;
    return 0;
}

igraph_integer_t iterated_greedy_run(const igraph_t *g, igraph_integer_t *color,
                                      const char *ordering, igraph_integer_t iterations)
{
    if (!ordering) ordering = "largest";
    igraph_integer_t n = igraph_vcount(g);
    igraph_integer_t *best = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *work = xmalloc((size_t)n * sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) { best[v] = -1; work[v] = -1; }
    igraph_integer_t best_colors = greedy_run(g, best);

    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    for (igraph_integer_t iter = 0; iter < iterations; iter++) {
        for (igraph_integer_t v = 0; v < n; v++) work[v] = -1;

        igraph_vector_int_t class_size;
        igraph_vector_int_init(&class_size, best_colors);
        for (igraph_integer_t v = 0; v < n; v++) VECTOR(class_size)[best[v]]++;

        igraph_vector_int_t class_order;
        igraph_vector_int_init(&class_order, best_colors);
        for (igraph_integer_t i = 0; i < best_colors; i++) VECTOR(class_order)[i] = i;

        if (strcmp(ordering, "largest") == 0) {
            idx_val_t *tmp = xmalloc((size_t)best_colors * sizeof(idx_val_t));
            for (igraph_integer_t i = 0; i < best_colors; i++) {
                tmp[i].index = i;
                tmp[i].value = VECTOR(class_size)[i];
            }
            cmp_class_sizes_ptr = &class_size;
            qsort(tmp, (size_t)best_colors, sizeof(idx_val_t), cmp_class_size_desc);
            for (igraph_integer_t i = 0; i < best_colors; i++) VECTOR(class_order)[i] = tmp[i].index;
            xfree(tmp);
        } else if (strcmp(ordering, "reverse") == 0) {
            for (igraph_integer_t i = 0; i < best_colors / 2; i++) {
                igraph_integer_t tmp = VECTOR(class_order)[i];
                VECTOR(class_order)[i] = VECTOR(class_order)[best_colors - 1 - i];
                VECTOR(class_order)[best_colors - 1 - i] = tmp;
            }
        } else {
            for (igraph_integer_t i = best_colors - 1; i > 0; i--) {
                igraph_integer_t j = rand() % (i + 1);
                igraph_integer_t tmp = VECTOR(class_order)[i];
                VECTOR(class_order)[i] = VECTOR(class_order)[j];
                VECTOR(class_order)[j] = tmp;
            }
        }

        igraph_vector_int_t perm;
        igraph_vector_int_init(&perm, n);
        igraph_integer_t pos = 0;
        for (igraph_integer_t ci = 0; ci < best_colors; ci++) {
            igraph_integer_t cls = VECTOR(class_order)[ci];
            for (igraph_integer_t v = 0; v < n; v++)
                if (best[v] == cls) VECTOR(perm)[pos++] = v;
        }

        for (igraph_integer_t i = 0; i < pos; i++) {
            igraph_integer_t v = VECTOR(perm)[i];
            if (igraph_neighbors(g, &neighbors, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
            }
            igraph_integer_t deg = igraph_vector_int_size(&neighbors);
            igraph_integer_t limit = deg + 1;
            bool *used = xcalloc((size_t)limit, sizeof(bool));
            for (igraph_integer_t j = 0; j < deg; j++) {
                igraph_integer_t w = VECTOR(neighbors)[j];
                if (work[w] >= 0 && work[w] < limit) used[work[w]] = true;
            }
            igraph_integer_t c = 0;
            while (used[c]) c++;
            work[v] = c;
            xfree(used);
        }

        igraph_integer_t work_colors = 0;
        for (igraph_integer_t v = 0; v < n; v++)
            if (work[v] >= work_colors) work_colors = work[v] + 1;

        if (work_colors < best_colors) {
            best_colors = work_colors;
            memcpy(best, work, (size_t)n * sizeof(igraph_integer_t));
        }

        igraph_vector_int_destroy(&perm);
        igraph_vector_int_destroy(&class_order);
        igraph_vector_int_destroy(&class_size);
    }

    igraph_vector_int_destroy(&neighbors);
    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    xfree(best);
    xfree(work);
    return best_colors;
}
