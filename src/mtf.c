#include "mtf.h"

#include "codes.h"
#include "util.h"

#include <stdlib.h>

void mtf_encode(const uint8_t *data, size_t len, bitwriter_t *w)
{
    uint8_t list[256];
    uint8_t pos[256];
    for (int i = 0; i < 256; i++) { list[i] = (uint8_t)i; pos[i] = (uint8_t)i; }

    for (int i = 0; i < 8; i++)
        bitwriter_write_bits(w, (len >> (56 - 8 * i)) & 0xFF, 8);

    for (size_t i = 0; i < len; i++) {
        uint8_t s = data[i];
        unsigned r = pos[s];
        bitwriter_write_bits(w, r, 8);
        /* move s to front */
        for (unsigned j = r; j > 0; j--) {
            list[j] = list[j - 1];
            pos[list[j]] = (uint8_t)j;
        }
        list[0] = s;
        pos[s] = 0;
    }
}

bool mtf_decode(const uint8_t *bits, size_t n_bytes, uint8_t **out, size_t *out_len)
{
    bitreader_t r;
    bitreader_init(&r, bits, n_bytes);

    uint64_t len = 0;
    for (int i = 0; i < 8; i++) len = (len << 8) | bitreader_read_bits(&r, 8);

    uint8_t list[256];
    uint8_t pos[256];
    for (int i = 0; i < 256; i++) { list[i] = (uint8_t)i; pos[i] = (uint8_t)i; }

    uint8_t *res = xmalloc(len ? len : 1);
    for (size_t i = 0; i < len; i++) {
        unsigned rk = (unsigned)bitreader_read_bits(&r, 8);
        uint8_t s = list[rk];
        res[i] = s;
        for (unsigned j = rk; j > 0; j--) {
            list[j] = list[j - 1];
            pos[list[j]] = (uint8_t)j;
        }
        list[0] = s;
        pos[s] = 0;
    }
    *out = res;
    *out_len = len;
    return true;
}

/* ---- RLE over the bit stream of `data` ---- */
void rle_encode(const uint8_t *data, size_t n_bytes, bitwriter_t *w)
{
    uint64_t n_bits = (uint64_t)n_bytes * 8;

    for (int i = 0; i < 8; i++)
        bitwriter_write_bits(w, (n_bits >> (56 - 8 * i)) & 0xFF, 8);

    bitreader_t in;
    bitreader_init(&in, data, n_bytes);

    /* runs alternate; the first run is a 1-run (possibly empty) */
    unsigned bit = 1;
    uint64_t len = 0, n_runs = 0;
    uint64_t *rl = NULL, rl_cap = 0;

    for (uint64_t i = 0; i < n_bits; i++) {
        unsigned b = bitreader_read(&in);
        if (b == bit) {
            len++;
        } else {
            if (n_runs == rl_cap) {
                rl_cap = rl_cap ? 2 * rl_cap : 16;
                rl = xrealloc(rl, rl_cap * sizeof(uint64_t));
            }
            rl[n_runs++] = len;
            bit = 1 - bit;
            len = 1;
        }
    }
    if (n_runs == rl_cap) {
        rl_cap = rl_cap ? 2 * rl_cap : 16;
        rl = xrealloc(rl, rl_cap * sizeof(uint64_t));
    }
    rl[n_runs++] = len;

    elias_gamma_encode(w, n_runs);
    for (uint64_t i = 0; i < n_runs; i++)
        elias_gamma_encode(w, rl[i] + 1);      /* +1 so we can store 0 */

    xfree(rl);
}

bool rle_decode(const uint8_t *bits, size_t n_bytes, uint8_t **out, size_t *out_len)
{
    bitreader_t r;
    bitreader_init(&r, bits, n_bytes);

    uint64_t n_bits = 0;
    for (int i = 0; i < 8; i++) n_bits = (n_bits << 8) | bitreader_read_bits(&r, 8);

    uint64_t n_runs = elias_gamma_decode(&r);
    uint8_t *res = xcalloc((n_bits + 7) / 8, 1);
    uint64_t idx = 0, bit = 1;
    for (uint64_t i = 0; i < n_runs && idx < n_bits; i++) {
        uint64_t run = elias_gamma_decode(&r) - 1;   /* decrement by 1 */
        for (uint64_t j = 0; j < run && idx < n_bits; j++) {
            if (bit) res[idx >> 3] |= (uint8_t)(1u << (7 - (idx & 7)));
            idx++;
        }
        bit = 1 - bit;
    }
    if (idx != n_bits) { xfree(res); return false; }
    *out = res;
    *out_len = (n_bits + 7) / 8;
    return true;
}
