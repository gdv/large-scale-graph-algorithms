#pragma once
#include <igraph.h>
#include <stdio.h>

void output_json(FILE *out,
                 const char *algorithm,
                 igraph_integer_t num_colors,
                 const igraph_integer_t *color,
                 igraph_integer_t n,
                 double time_ms,
                 igraph_integer_t conflicts);
