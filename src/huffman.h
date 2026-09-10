#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "bitio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Huffman coding of a byte stream.
 *
 * Stream format:
 *   - 64 bits: original length in bytes
 *   - 256 * 5 bits: code length of each byte (0 = unused)
 *   - data: canonical Huffman codes (MSB first)
 *
 * freq[] must be the symbol frequencies of `data` (the caller computes or
 * provides them).  Returns false on empty input. Decoders rebuild the
 * canonical trie from the header. */
bool huffman_encode(const uint8_t *data, size_t len, const uint64_t freq[256],
                    bitwriter_t *out);
bool huffman_decode(const uint8_t *bits, size_t n_bytes,
                    uint8_t **out, size_t *out_len);
#endif
