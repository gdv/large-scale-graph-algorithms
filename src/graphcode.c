#include "graphcode.h"

#include "codes.h"
#include "util.h"

#include <stdlib.h>

/* signed gap: sign bit, then gamma(|g|+1) */
static void write_gap(bitwriter_t *w, igraph_integer_t g)
{
    bitwriter_write(w, g < 0 ? 1 : 0);
    uint64_t mag = (uint64_t)labs((long)g);
    elias_gamma_encode(w, mag + 1);                        /* gamma(|g|+1) */
}

static igraph_integer_t read_gap(bitreader_t *r)
{
    unsigned neg = bitreader_read(r);
    uint64_t mag = elias_gamma_decode(r);
    igraph_integer_t g = (igraph_integer_t)mag - 1;
    return neg ? -g : g;
}

void gap_encode_list(bitwriter_t *w, igraph_integer_t v,
                     const igraph_integer_t *neighbors, igraph_integer_t deg)
{
    if (deg <= 0) return;
    write_gap(w, neighbors[0] - v);
    for (igraph_integer_t i = 1; i < deg; i++)
        write_gap(w, neighbors[i] - neighbors[i - 1]);
}

void gap_decode_list(bitreader_t *r, igraph_integer_t v, igraph_integer_t deg,
                     igraph_integer_t *out)
{
    if (deg <= 0) return;
    out[0] = v + read_gap(r);
    for (igraph_integer_t i = 1; i < deg; i++)
        out[i] = out[i - 1] + read_gap(r);
}

void ref_encode_list(bitwriter_t *w, igraph_integer_t prev_v, igraph_integer_t v,
                     const igraph_integer_t *ref, igraph_integer_t ref_deg,
                     const igraph_integer_t *cur, igraph_integer_t cur_deg)
{
    elias_gamma_encode(w, (uint64_t)prev_v + 1);

    /* characteristic vector: ref[i] present in cur? */
    igraph_integer_t i = 0, j = 0;
    while (i < ref_deg) {
        while (j < cur_deg && cur[j] < ref[i]) j++;
        int present = (j < cur_deg && cur[j] == ref[i]);
        bitwriter_write(w, present ? 0 : 1);
        i++;
    }

    /* extras: cur \ ref */
    igraph_integer_t *extra = xmalloc((size_t)cur_deg * sizeof(igraph_integer_t));
    igraph_integer_t ne = 0;
    i = 0; j = 0;
    while (j < cur_deg) {
        while (i < ref_deg && ref[i] < cur[j]) i++;
        if (i >= ref_deg || ref[i] != cur[j]) extra[ne++] = cur[j];
        j++;
    }
    elias_gamma_encode(w, (uint64_t)ne + 1);
    gap_encode_list(w, v, extra, ne);
    xfree(extra);
}

void ref_decode_list(bitreader_t *r, igraph_integer_t prev_v, igraph_integer_t v,
                     const igraph_integer_t *ref, igraph_integer_t ref_deg,
                     igraph_integer_t *out, igraph_integer_t *out_deg)
{
    (void)prev_v;
    elias_gamma_decode(r);                      /* consume prev_v */

    igraph_integer_t *base = xmalloc((size_t)ref_deg * sizeof(igraph_integer_t));
    igraph_integer_t nb = 0;
    for (igraph_integer_t i = 0; i < ref_deg; i++)
        if (bitreader_read(r) == 0) base[nb++] = ref[i];

    igraph_integer_t ne = (igraph_integer_t)elias_gamma_decode(r) - 1;
    igraph_integer_t *extra = xmalloc((size_t)(ne ? ne : 1) * sizeof(igraph_integer_t));
    gap_decode_list(r, v, ne, extra);

    /* merge base and extra, both sorted */
    igraph_integer_t i = 0, j = 0, k = 0;
    while (i < nb && j < ne)
        out[k++] = (base[i] < extra[j]) ? base[i++] : extra[j++];
    while (i < nb) out[k++] = base[i++];
    while (j < ne) out[k++] = extra[j++];
    *out_deg = k;
    xfree(base);
    xfree(extra);
}

void interval_encode_list(bitwriter_t *w, igraph_integer_t v,
                          const igraph_integer_t *sorted, igraph_integer_t deg)
{
    if (deg <= 0) return;
    igraph_integer_t n_int = 1;
    for (igraph_integer_t i = 1; i < deg; i++)
        if (sorted[i] != sorted[i - 1] + 1) n_int++;

    elias_gamma_encode(w, (uint64_t)n_int);
    igraph_integer_t prev_end = 0;
    igraph_integer_t i = 0;
    for (igraph_integer_t it = 0; it < n_int; it++) {
        igraph_integer_t b = sorted[i];
        igraph_integer_t e = b;
        while (i + 1 < deg && sorted[i + 1] == e + 1) { e++; i++; }
        i++;
        if (it == 0) {
            write_gap(w, b - v);                       /* signed, gamma(|g|+1) */
        } else {
            elias_gamma_encode(w, (uint64_t)(b - prev_end)); /* >= 2, gamma ok */
        }
        elias_gamma_encode(w, (uint64_t)(e - b) + 1);  /* length = e-b, +1 */
        prev_end = e;
    }
}

void interval_decode_list(bitreader_t *r, igraph_integer_t v,
                          igraph_integer_t *out, igraph_integer_t *out_deg)
{
    igraph_integer_t n_int = (igraph_integer_t)elias_gamma_decode(r);
    igraph_integer_t k = 0, prev_end = 0;
    for (igraph_integer_t it = 0; it < n_int; it++) {
        igraph_integer_t b;
        if (it == 0) {
            b = v + read_gap(r);
        } else {
            b = prev_end + (igraph_integer_t)elias_gamma_decode(r);
        }
        igraph_integer_t len = (igraph_integer_t)elias_gamma_decode(r) - 1;
        for (igraph_integer_t x = b; x <= b + len; x++) out[k++] = x;
        prev_end = b + len;
    }
    *out_deg = k;
}
