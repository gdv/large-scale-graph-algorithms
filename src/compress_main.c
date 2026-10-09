#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitio.h"
#include "codes.h"
#include "graph_io.h"
#include "graphcode.h"
#include "huffman.h"
#include "mtf.h"
#include "output.h"
#include "util.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s -i FILE -a ALGO\n"
        "  ALGO: huffman | mtf | rle | gaps | reference | interval\n"
        "        | eliasg | eliasd | nibble | minbin | zetak\n", prog);
    exit(1);
}

static int cmp_int(const void *a, const void *b)
{
    igraph_integer_t x = *(const igraph_integer_t *)a;
    igraph_integer_t y = *(const igraph_integer_t *)b;
    return (x > y) - (x < y);
}

/* Collect the flattened adjacency values of the graph into `out`. */
static uint8_t *flatten_adjacency(const igraph_t *g, igraph_integer_t n,
                                  size_t *out_len)
{
    uint8_t *flat = NULL;
    size_t len = 0, cap = 0;
    for (igraph_integer_t u = 0; u < n; u++) {
        igraph_vector_int_t nb;
        igraph_vector_int_init(&nb, 0);
        igraph_neighbors(g, &nb, u, IGRAPH_OUT, IGRAPH_NO_LOOPS, false);
        for (igraph_integer_t i = 0; i < (igraph_integer_t)igraph_vector_int_size(&nb); i++) {
            if (len == cap) {
                cap = cap ? 2 * cap : 1024;
                flat = xrealloc(flat, cap);
            }
            flat[len++] = (uint8_t)(VECTOR(nb)[i] & 0xFF);
        }
        igraph_vector_int_destroy(&nb);
    }
    *out_len = len;
    return flat;
}

int main(int argc, char **argv)
{
    const char *input = NULL, *algo = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) input = argv[++i];
        else if (!strcmp(argv[i], "-a") && i + 1 < argc) algo = argv[++i];
        else usage(argv[0]);
    }
    if (!input || !algo) usage(argv[0]);
    if (strcmp(algo, "huffman") && strcmp(algo, "mtf") && strcmp(algo, "rle") &&
        strcmp(algo, "gaps") && strcmp(algo, "reference") && strcmp(algo, "interval") &&
        strcmp(algo, "eliasg") && strcmp(algo, "eliasd") && strcmp(algo, "nibble") &&
        strcmp(algo, "minbin") && strcmp(algo, "zetak"))
        usage(argv[0]);

    igraph_set_attribute_table(&igraph_cattribute_table);
    phase_begin("read");
    igraph_t g = read_graph_or_die(input, 0, true);
    igraph_integer_t n = (igraph_integer_t)igraph_vcount(&g);
    igraph_integer_t m = (igraph_integer_t)igraph_ecount(&g);
    phase_end();
    phase_begin("encode");

    bitwriter_t w;
    bitwriter_init(&w);

    if (!strcmp(algo, "huffman") || !strcmp(algo, "mtf") || !strcmp(algo, "rle")) {
        size_t flat_len = 0;
        uint8_t *flat = flatten_adjacency(&g, n, &flat_len);
        if (!strcmp(algo, "huffman")) {
            uint64_t freq[256] = {0};
            for (size_t i = 0; i < flat_len; i++) freq[flat[i]]++;
            if (!huffman_encode(flat, flat_len, freq, &w)) {
                fprintf(stderr, "empty input\n");
                xfree(flat);
                return 1;
            }
        } else if (!strcmp(algo, "mtf")) {
            mtf_encode(flat, flat_len, &w);
        } else {
            rle_encode(flat, flat_len, &w);
        }
        xfree(flat);
    } else if (!strcmp(algo, "gaps") || !strcmp(algo, "reference") ||
               !strcmp(algo, "interval")) {
        /* sorted adjacency lists per vertex */
        igraph_integer_t **adj = xmalloc((size_t)n * sizeof(igraph_integer_t *));
        igraph_integer_t *deg = xcalloc((size_t)n, sizeof(igraph_integer_t));
        for (igraph_integer_t u = 0; u < n; u++) {
            igraph_vector_int_t nb;
            igraph_vector_int_init(&nb, 0);
            igraph_neighbors(&g, &nb, u, IGRAPH_OUT, IGRAPH_NO_LOOPS, false);
            igraph_integer_t deg_u = (igraph_integer_t)igraph_vector_int_size(&nb);
            deg[u] = deg_u;
            adj[u] = xmalloc((size_t)(deg_u ? deg_u : 1) * sizeof(igraph_integer_t));
            for (igraph_integer_t i = 0; i < deg_u; i++) adj[u][i] = VECTOR(nb)[i];
            qsort(adj[u], (size_t)deg_u, sizeof(igraph_integer_t), cmp_int);
            igraph_vector_int_destroy(&nb);
        }
        for (igraph_integer_t u = 0; u < n; u++) {
            if (!strcmp(algo, "gaps")) {
                gap_encode_list(&w, u, adj[u], deg[u]);
            } else if (!strcmp(algo, "reference")) {
                igraph_integer_t prev = (u > 0) ? u - 1 : 0;
                if (u > 0)
                    ref_encode_list(&w, prev, u, adj[prev], deg[prev], adj[u], deg[u]);
                else
                    gap_encode_list(&w, u, adj[u], deg[u]);
            } else {
                interval_encode_list(&w, u, adj[u], deg[u]);
            }
        }
        for (igraph_integer_t u = 0; u < n; u++) xfree(adj[u]);
        xfree(adj);
        xfree(deg);
    } else {
        /* integer codes over adjacency values */
        igraph_vector_int_t nb;
        igraph_vector_int_init(&nb, 0);
        for (igraph_integer_t u = 0; u < n; u++) {
            igraph_neighbors(&g, &nb, u, IGRAPH_OUT, IGRAPH_NO_LOOPS, false);
            for (igraph_integer_t i = 0; i < (igraph_integer_t)igraph_vector_int_size(&nb); i++) {
                uint64_t x = (uint64_t)VECTOR(nb)[i];
                if (!strcmp(algo, "eliasg"))      elias_gamma_encode(&w, x + 1);
                else if (!strcmp(algo, "eliasd")) elias_delta_encode(&w, x + 1);
                else if (!strcmp(algo, "nibble")) nibble_encode(&w, x);
                else if (!strcmp(algo, "minbin")) minbin_encode(&w, x, (uint64_t)n);
                else                              zeta_encode(&w, x + 1, 2);
            }
        }
        igraph_vector_int_destroy(&nb);
    }

    size_t out_bits = bitwriter_nbits(&w);
    bitwriter_finish(&w);
    size_t in_bytes = (size_t)m * 8;   /* naive baseline: 8 bytes per arc */
    phase_end();
    printf("{\"algorithm\": \"%s\", \"n\": %" IGRAPH_PRId ", \"m\": %" IGRAPH_PRId
           ", \"in_bytes\": %zu, \"out_bytes\": %zu, \"ratio\": %.3f"
           ", \"bits_per_edge\": %.3f",
           algo, n, m, in_bytes, w.n_bytes,
           (double)w.n_bytes / (double)(in_bytes ? in_bytes : 1),
           m ? (double)out_bits / (double)m : 0.0);
    metrics_json(stdout);
    printf("\n}\n");
    bitwriter_destroy(&w);
    igraph_destroy(&g);
    return 0;
}
