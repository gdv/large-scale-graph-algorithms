#pragma once
#include <igraph.h>

igraph_integer_t sa1_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out);

igraph_integer_t sa2_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out);
