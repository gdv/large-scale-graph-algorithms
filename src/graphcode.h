#ifndef GRAPHCODE_H
#define GRAPHCODE_H

#include "bitio.h"

#include <igraph.h>

/* Coding schemes for (sorted) adjacency lists, following the course slides.
 * Each pair of functions is a codec for ONE list; the caller orchestrates
 * the per-vertex application (as bin/compress does). */

/* Gap representation: differences w1-v, then consecutive gaps. */
void gap_encode_list(bitwriter_t *w, igraph_integer_t v,
                     const igraph_integer_t *neighbors, igraph_integer_t deg);
void gap_decode_list(bitreader_t *r, igraph_integer_t v, igraph_integer_t deg,
                     igraph_integer_t *out);

/* Reference compression: <previous vertex, bit vector of ref\cur,
 * gap-encoded cur\ref>.  ref must be the previous adjacency list. */
void ref_encode_list(bitwriter_t *w, igraph_integer_t prev_v, igraph_integer_t v,
                     const igraph_integer_t *ref, igraph_integer_t ref_deg,
                     const igraph_integer_t *cur, igraph_integer_t cur_deg);
void ref_decode_list(bitreader_t *r, igraph_integer_t prev_v, igraph_integer_t v,
                     const igraph_integer_t *ref, igraph_integer_t ref_deg,
                     igraph_integer_t *out, igraph_integer_t *out_deg);

/* Interval encoding: maximal consecutive runs [b, b+L]. */
void interval_encode_list(bitwriter_t *w, igraph_integer_t v,
                          const igraph_integer_t *sorted, igraph_integer_t deg);
void interval_decode_list(bitreader_t *r, igraph_integer_t v,
                          igraph_integer_t *out, igraph_integer_t *out_deg);
#endif
