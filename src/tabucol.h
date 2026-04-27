#pragma once
#include <igraph.h>

igraph_integer_t tabucol_run(const igraph_t *g, igraph_integer_t *color,
                              igraph_integer_t num_colors,
                              igraph_integer_t iterations,
                              igraph_integer_t tenure,
                              igraph_integer_t *conflicts_out);
