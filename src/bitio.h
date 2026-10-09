#ifndef BITIO_H
#define BITIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* MSB-first bit writer over a growable byte buffer.  This is the sound
 * foundation of every bit-level codec in this module: all encoders write
 * through it and all decoders read through it, so the bit order convention
 * is defined exactly once. */
typedef struct {
    uint8_t *buf;        /* encoded bytes */
    size_t   n_bytes;    /* bytes written */
    size_t   cap;
    unsigned bit_pos;    /* next bit position 0..7 (MSB first) */
} bitwriter_t;

void bitwriter_init(bitwriter_t *w);
void bitwriter_destroy(bitwriter_t *w);
void bitwriter_write(bitwriter_t *w, unsigned bit);
void bitwriter_write_bits(bitwriter_t *w, uint64_t value, unsigned nbits);
/* Bits actually written so far, ignoring the zero padding of the current
 * byte. This -- not n_bytes -- is what a compression ratio should be based
 * on. */
size_t bitwriter_nbits(const bitwriter_t *w);
/* pad the current byte with zero bits; buffer is then ready to read. */
void bitwriter_finish(bitwriter_t *w);

typedef struct {
    const uint8_t *buf;
    size_t   n_bytes;
    size_t   byte_pos;
    unsigned bit_pos;
} bitreader_t;

void bitreader_init(bitreader_t *r, const uint8_t *buf, size_t n_bytes);
unsigned bitreader_read(bitreader_t *r);             /* 0 past the end */
uint64_t bitreader_read_bits(bitreader_t *r, unsigned nbits);
bool bitreader_eof(bitreader_t *r);
#endif
