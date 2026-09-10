#ifndef MTF_H
#define MTF_H

#include "bitio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Move-to-front transform over the 256-byte alphabet.
 * Stream format: 64-bit length, then 8-bit ranks. */
void mtf_encode(const uint8_t *data, size_t len, bitwriter_t *w);
bool mtf_decode(const uint8_t *bits, size_t n_bytes, uint8_t **out, size_t *out_len);

/* Run-length encoding of a bit stream (per the slides: runs alternate
 * starting with a 1-run, which may be empty; run lengths are decremented
 * by 1 on decode).  Stream format: 64-bit number of bits, then gamma(run
 * count), then gamma(length+1) per run. */
void rle_encode(const uint8_t *data, size_t n_bytes, bitwriter_t *w);
bool rle_decode(const uint8_t *bits, size_t n_bytes, uint8_t **out, size_t *out_len);
#endif
