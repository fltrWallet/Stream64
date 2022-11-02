//===----------------------------------------------------------------------===//
//
// This source file is part of the Stream64 open source project
//
// Copyright (c) 2022 fltrWallet AG and the Stream64 project authors
// Licensed under Apache License v2.0
//
// See LICENSE.md for license information
//
// SPDX-License-Identifier: Apache-2.0
//
//===----------------------------------------------------------------------===//
#include<stdint.h>
#include<stddef.h>
#include<assert.h>

#include "CStream64.h"

__attribute__ ((__always_inline__)) static inline uint64_t read64be(const uint8_t *byteptr) {
    uint64_t load __attribute__((aligned (1))) = *(uint64_t *)byteptr;
    return __builtin_bswap64(load);
}
// portable alternative
/*
 uint64_t load64_le(uint8_t const* V)
 {
    uint64_t Ret = 0;
    Ret |= (uint64_t) V[0];
    Ret |= ((uint64_t) V[1]) << 8;
    Ret |= ((uint64_t) V[2]) << 16;
    Ret |= ((uint64_t) V[3]) << 24;
    Ret |= ((uint64_t) V[4]) << 32;
    Ret |= ((uint64_t) V[5]) << 40;
    Ret |= ((uint64_t) V[6]) << 48;
    Ret |= ((uint64_t) V[7]) << 56;
    return Ret;
 }
 */

//static inline void printb(uint64_t v) {
//    uint64_t i, s = 1ull<<((sizeof(v)<<3)-1); // s = only most significant bit at 1
//    for (i = s; i; i>>=1) printf("%d", v & i || 0 );
//}

static inline void refill3(uint64_t *const bitbuf,
                           size_t *const bitpos,
                           const uint8_t *const base,
                           size_t *const pos) {
    assert(*bitpos <= 64);

    // Advance the pointer by however many full bytes we consumed
    *pos += *bitpos >> 3;

    // Refill
    uint8_t const *current = base + *pos;
    *bitbuf = read64be(current);

    // Number of bits in the current byte we've already consumed
    // (we took care of the full bytes; these are the leftover
    // bits that didn't make a full byte.)
    *bitpos &= 7;
}

static inline uint64_t peekbits3(size_t count,
                                 uint64_t bitbuf,
                                 const size_t bitpos) {
    assert(bitpos + count <= 64);
    assert(count >= 1 && count <= 64 - 7);

    // Shift out the bits we've already consumed
    uint64_t remaining = bitbuf << bitpos;

    // Return the top "count" bits
    return remaining >> (64 - count);
}

static inline void consume3(size_t count, size_t *const bitpos) {
    *bitpos += count;
}

static inline uint64_t getbits3(size_t count,
                                uint64_t *bitbuf,
                                size_t *bitpos,
                                const uint8_t *const bitptr,
                                size_t *pos) {
    refill3(bitbuf, bitpos, bitptr, pos);
    uint64_t result = peekbits3(count, *bitbuf, *bitpos);
    consume3(count, bitpos);

    return result;
}

inline gcs_state initialize() {
    gcs_state state;
    
    state.bitbuf = 0;
    state.bitpos = 0;
    state.bufpos = 0;
    
    return state;
}

static inline uint64_t readUnary(const uint8_t *const byteptr, gcs_state *const state) {
    uint64_t next, result = 0;

    while ((next = getbits3(1, &state->bitbuf, &state->bitpos, byteptr, &state->bufpos)) == GCS_UNARY) {
        result += next;
    }
    
    return result;
}

inline uint64_t read_gcs_next(const uint8_t *const byteptr, const size_t p, gcs_state *const state) {
    uint64_t upper = readUnary(byteptr, state);
    uint64_t lower = getbits3(p, &state->bitbuf, &state->bitpos, byteptr, &state->bufpos);
    
    return (upper << p) + lower;
}

inline uint64_t read_p(const uint8_t *const byteptr, const size_t p, gcs_state *const state) {
    return getbits3(p, &state->bitbuf, &state->bitpos, byteptr, &state->bufpos);
}

// MARK: Write
static inline void write_byte_be(const uint64_t data,
                              const size_t byte_number,
                              uint8_t *dest_ptr) {
    size_t index = 7 - byte_number;
    uint8_t result = (uint8_t)(data >> ((size_t) 8) * index);
    
    *dest_ptr = result;
}

static inline void write_uint64(const uint64_t __attribute__((aligned (1))) data,
                                uint8_t *dest_ptr) {
    uint64_t __attribute__((aligned (1))) *rebase = (uint64_t *)dest_ptr;
    *rebase = __builtin_bswap64(data);
}

static inline void refill_write(uint64_t *const bits_container,
                                size_t *const bits_position,
                                uint8_t *const dest_ptr,
                                size_t *dest_offset) {
    assert(*bits_position < 64);
    size_t full_bytes = *bits_position >> 3;

    for (size_t i = 0; i < full_bytes; ++i) {
        write_byte_be(*bits_container, i, dest_ptr + *dest_offset + i);
    }

    // Forward the destination offset
    *dest_offset += full_bytes;
    // Reset the bits container
    *bits_container <<= full_bytes * 8;
    // Number of bits in the current byte we've already consumed
    // (we took care of the full bytes; these are the leftover
    // bits that didn't make a full byte.)
    *bits_position &= 7;
}

static inline void bits_write(const uint64_t value,
                              size_t count,
                              uint64_t *bits_container,
                              const size_t bits_position) {
    assert((value >> count) == 0);
    
    uint64_t leftShift = value << (64 - count);
    *bits_container |= leftShift >> bits_position;
}

static inline void putbits(const uint64_t value,
                           size_t count,
                           uint64_t *bits_container,
                           size_t *bits_position,
                           uint8_t *const dest_ptr,
                           size_t *dest_offset) {
    assert(value >> count == 0);
    refill_write(bits_container, bits_position, dest_ptr, dest_offset);
    bits_write(value, count, bits_container, *bits_position);
    consume3(count, bits_position);
}

static inline void finalize_write(uint64_t bits_container,
                                  size_t bits_position,
                                  uint8_t *const dest_ptr,
                                  size_t *dest_offset) {
    refill_write(&bits_container, &bits_position, dest_ptr, dest_offset);

    if (bits_position > 0) {
        uint8_t *next = dest_ptr + *dest_offset;
        *next = (uint8_t)(bits_container >> 56);
        (*dest_offset)++;
    }
}

inline uint32_t write_bitstream(const uint64_t *const data,
                                const size_t data_size,
                                const size_t p,
                                uint8_t *const dest_ptr,
                                size_t *result_bytes) {
    if (p > 56) {
        return 1;
    }
    
    uint64_t bits_container = 0;
    size_t bits_position = 0;
    size_t dest_offset = 0;

    for (size_t i = 0; i < data_size; ++i) {
        putbits(*(data + i), p, &bits_container, &bits_position, dest_ptr, &dest_offset);
    }

    finalize_write(bits_container, bits_position, dest_ptr, &dest_offset);
    
    if (dest_offset > 0) {
        *result_bytes = dest_offset;
    } else {
        return 1;
    }
    
    return 0;
}

inline uint32_t write_gcs(const size_t count,
                          const uint64_t *const sorted_data,
                          const size_t data_size,
                          uint8_t *const dest_ptr,
                          size_t *result_bytes) {
    size_t lower_mask = (1 << (count)) - 1;
    
    uint64_t accumulator = 0ULL, bits_container = 0ULL;
    size_t bits_position = 0, dest_offset = 0;
    
    for (size_t i = 0; i < data_size; ++i) {
        uint64_t next = *(sorted_data + i) - accumulator;
        accumulator += next;
        uint64_t lower = next & lower_mask;
        uint64_t upper = next >> count;

        if (upper > 0) {
            for (uint64_t acc = 0; acc < upper; ++acc) {
                putbits(GCS_UNARY, 1, &bits_container, &bits_position, dest_ptr, &dest_offset);
            }
        }
        putbits(GCS_UNARY_EOF, 1, &bits_container, &bits_position, dest_ptr, &dest_offset);

        putbits(lower, count, &bits_container, &bits_position, dest_ptr, &dest_offset);

        if (dest_offset >= (GCS_BUFFER_WRITE - 1024)) {
            return 1;
        }
    }

    finalize_write(bits_container, bits_position, dest_ptr, &dest_offset);
    
    if (dest_offset > 0) {
        *result_bytes = dest_offset;
    } else {
        return 1;
    }
    
    return 0;
}
