#include "bitio.h"

#include <stdlib.h>

#include "util.h"

void bitwriter_init(bitwriter_t *w)
{
    w->cap = 64;
    w->buf = xmalloc(w->cap);
    w->n_bytes = 0;
    w->bit_pos = 0;
}

void bitwriter_destroy(bitwriter_t *w)
{
    free(w->buf);
    w->buf = NULL;
}

void bitwriter_write(bitwriter_t *w, unsigned bit)
{
    if (w->bit_pos == 0) {
        if (w->n_bytes == w->cap) {
            w->cap *= 2;
            w->buf = xrealloc(w->buf, w->cap);
        }
        w->buf[w->n_bytes] = 0;
        w->n_bytes++;
    }
    if (bit) w->buf[w->n_bytes - 1] |= (uint8_t)(1u << (7 - w->bit_pos));
    w->bit_pos = (w->bit_pos + 1) & 7;
}

void bitwriter_write_bits(bitwriter_t *w, uint64_t value, unsigned nbits)
{
    for (unsigned i = nbits; i-- > 0;)
        bitwriter_write(w, (unsigned)((value >> i) & 1));
}

void bitwriter_finish(bitwriter_t *w)
{
    w->bit_pos = 0;   /* padding zeros already in the buffer */
}

void bitreader_init(bitreader_t *r, const uint8_t *buf, size_t n_bytes)
{
    r->buf = buf;
    r->n_bytes = n_bytes;
    r->byte_pos = 0;
    r->bit_pos = 0;
}

unsigned bitreader_read(bitreader_t *r)
{
    if (r->byte_pos >= r->n_bytes) return 0;
    unsigned bit = (r->buf[r->byte_pos] >> (7 - r->bit_pos)) & 1;
    if (++r->bit_pos == 8) { r->bit_pos = 0; r->byte_pos++; }
    return bit;
}

uint64_t bitreader_read_bits(bitreader_t *r, unsigned nbits)
{
    uint64_t v = 0;
    for (unsigned i = 0; i < nbits; i++) v = (v << 1) | bitreader_read(r);
    return v;
}

bool bitreader_eof(bitreader_t *r)
{
    return r->byte_pos >= r->n_bytes;
}
