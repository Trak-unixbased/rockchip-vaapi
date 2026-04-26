/*
 * bs.h — inline bitstream writer for H.264 Exp-Golomb encoding
 *
 * Copyright (C) 2026 Eduardo García-Mádico Portabella <woodyst@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t   capacity;
    size_t   byte_pos;
    int      bit_pos;
} BSWriter;

static inline void bs_init(BSWriter *bs, uint8_t *buf, size_t cap) {
    bs->data = buf; bs->capacity = cap;
    bs->byte_pos = 0; bs->bit_pos = 0;
    memset(buf, 0, cap);
}

static inline void bs_write1(BSWriter *bs, int bit) {
    if (bs->byte_pos >= bs->capacity) return;
    bs->data[bs->byte_pos] |= (bit & 1) << (7 - bs->bit_pos);
    if (++bs->bit_pos == 8) { bs->bit_pos = 0; bs->byte_pos++; }
}

static inline void bs_write(BSWriter *bs, uint32_t val, int n) {
    for (int i = n - 1; i >= 0; i--) bs_write1(bs, (val >> i) & 1);
}

static inline void bs_write_ue(BSWriter *bs, uint32_t val) {
    val++;
    int bits = 0;
    uint32_t tmp = val;
    while (tmp > 1) { tmp >>= 1; bits++; }
    bs_write(bs, 0, bits);
    bs_write(bs, val, bits + 1);
}

static inline void bs_write_se(BSWriter *bs, int32_t val) {
    uint32_t uval = (val <= 0) ? (uint32_t)(-2 * val) : (uint32_t)(2 * val - 1);
    bs_write_ue(bs, uval);
}

static inline void bs_rbsp_trailing(BSWriter *bs) {
    bs_write1(bs, 1);
    while (bs->bit_pos != 0) bs_write1(bs, 0);
}

static inline size_t bs_bytes(const BSWriter *bs) {
    return bs->byte_pos + (bs->bit_pos > 0 ? 1 : 0);
}
