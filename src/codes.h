#ifndef CODES_H
#define CODES_H

#include "bitio.h"

#include <stdint.h>

/* All codes follow the course slide definitions exactly (x >= 1 unless
 * stated otherwise).  Each *encode writes to a bitwriter and each *decode
 * reads back from a bitreader, so codecs compose freely. */

void elias_gamma_encode(bitwriter_t *w, uint64_t x);
uint64_t elias_gamma_decode(bitreader_t *r);

void elias_delta_encode(bitwriter_t *w, uint64_t x);
uint64_t elias_delta_decode(bitreader_t *r);

/* Nibble code: 3-bit blocks with a continuation marker (1 on the last).
 * x >= 0. */
void nibble_encode(bitwriter_t *w, uint64_t x);
uint64_t nibble_decode(bitreader_t *r);

/* Minimal binary over a universe of size z (0 <= x <= z-1, z >= 1). */
void minbin_encode(bitwriter_t *w, uint64_t x, uint64_t z);
uint64_t minbin_decode(bitreader_t *r, uint64_t z);

/* zeta_k: k-shrinking factor, x >= 1. */
void zeta_encode(bitwriter_t *w, uint64_t x, unsigned k);
uint64_t zeta_decode(bitreader_t *r, unsigned k);
#endif
