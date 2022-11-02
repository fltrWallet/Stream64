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
#ifndef INCLUDE_CSTREAM64_H
#define INCLUDE_CSTREAM64_H

#include<stdint.h>

typedef struct gcs_state {
    uint64_t bitbuf;
    size_t bitpos;
    size_t bufpos;
} gcs_state;

// BitStream
extern inline gcs_state initialize();
extern inline uint64_t read_p(const uint8_t *const, const size_t, gcs_state *const);
extern inline uint32_t write_bitstream(const uint64_t *const, const size_t, const size_t, uint8_t *const, size_t*);

// GCS
extern inline uint64_t read_gcs_next(const uint8_t *const, const size_t, gcs_state *const);
extern inline uint32_t write_gcs(const size_t, const uint64_t *const, const size_t, uint8_t *const, size_t*);

#define GCS_BUFFER_WRITE 4 * 1024 * 1024
#define GCS_UNARY 0x0000000000000001ULL
#define GCS_UNARY_EOF 0x0000000000000000ULL

#endif /* INCLUDE_CSTREAM64_H */
