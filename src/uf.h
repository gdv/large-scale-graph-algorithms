#ifndef UF_H
#define UF_H

#include <igraph.h>

/* Union-Find with path compression and union by rank. */
typedef struct {
    igraph_integer_t n;
    igraph_integer_t *parent;
    igraph_integer_t *rank;
} uf_t;

void uf_init(uf_t *u, igraph_integer_t n);
igraph_integer_t uf_find(uf_t *u, igraph_integer_t x);
void uf_union(uf_t *u, igraph_integer_t a, igraph_integer_t b);
void uf_destroy(uf_t *u);
#endif
