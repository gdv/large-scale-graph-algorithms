#include "codes.h"

#include <stdlib.h>

static unsigned floor_log2(uint64_t x)
{
    unsigned n = 0;
    while (x >>= 1) n++;
    return n;
}

void elias_gamma_encode(bitwriter_t *w, uint64_t x)
{
    unsigned n = floor_log2(x);
    for (unsigned i = 0; i < n; i++) bitwriter_write(w, 0);
    bitwriter_write_bits(w, x, n + 1);
}

uint64_t elias_gamma_decode(bitreader_t *r)
{
    unsigned n = 0;
    while (bitreader_read(r) == 0) n++;
    return ((uint64_t)1 << n) | bitreader_read_bits(r, n);
}

void elias_delta_encode(bitwriter_t *w, uint64_t x)
{
    unsigned n = floor_log2(x);
    elias_gamma_encode(w, (uint64_t)n + 1);
    bitwriter_write_bits(w, x ^ ((uint64_t)1 << n), n);   /* drop leading 1 */
}

uint64_t elias_delta_decode(bitreader_t *r)
{
    uint64_t l = elias_gamma_decode(r);                   /* n + 1 */
    unsigned n = (unsigned)l - 1;
    return ((uint64_t)1 << n) | bitreader_read_bits(r, n);
}

void nibble_encode(bitwriter_t *w, uint64_t x)
{
    unsigned nbits = 0;
    uint64_t t = x;
    while (t) { nbits++; t >>= 1; }
    if (nbits == 0) nbits = 1;
    unsigned rem = nbits % 3;
    if (rem) nbits += 3 - rem;

    for (unsigned i = nbits; i > 0; i -= 3) {
        unsigned block = (unsigned)((x >> (i - 3)) & 7);
        unsigned marker = (i == 3) ? 1 : 0;               /* last block */
        bitwriter_write_bits(w, (marker << 3) | block, 4);
    }
}

uint64_t nibble_decode(bitreader_t *r)
{
    uint64_t x = 0;
    for (;;) {
        unsigned byte = (unsigned)bitreader_read_bits(r, 4);
        x = (x << 3) | (byte & 7);
        if (byte & 8) break;
    }
    return x;
}

void minbin_encode(bitwriter_t *w, uint64_t x, uint64_t z)
{
    if (z <= 1) return;
    unsigned s = 0;
    while (((uint64_t)1 << s) < z) s++;                   /* ceil(log2 z) */
    uint64_t p = ((uint64_t)1 << s) - z;
    if (x < p)      bitwriter_write_bits(w, x, s - 1);
    else            bitwriter_write_bits(w, x + p, s);
}

uint64_t minbin_decode(bitreader_t *r, uint64_t z)
{
    if (z <= 1) return 0;
    unsigned s = 0;
    while (((uint64_t)1 << s) < z) s++;
    uint64_t p = ((uint64_t)1 << s) - z;
    uint64_t v = bitreader_read_bits(r, s - 1);
    if (v < p) return v;
    v = (v << 1) | bitreader_read(r);
    return v - p;
}

void zeta_encode(bitwriter_t *w, uint64_t x, unsigned k)
{
    uint64_t h = 0, lower = 1, upper = (uint64_t)1 << k;
    while (x >= upper) { h++; lower = upper; upper <<= k; }
    for (uint64_t i = 0; i < h; i++) bitwriter_write(w, 0);
    bitwriter_write(w, 1);
    minbin_encode(w, x - lower, upper - lower);
}

uint64_t zeta_decode(bitreader_t *r, unsigned k)
{
    uint64_t h = 0;
    while (bitreader_read(r) == 0) h++;
    uint64_t lower = (uint64_t)1 << (h * k);
    uint64_t upper = lower << k;
    return lower + minbin_decode(r, upper - lower);
}
