// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2018, Intel Corporation */

/*
 * map_hashmap_tx_unsafe.h -- common interface for maps
 */

#ifndef MAP_HASHMAP_TX_UNSAFE_H
#define MAP_HASHMAP_TX_UNSAFE_H

#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct map_ops hashmap_tx_unsafe_ops;

#define MAP_HASHMAP_TX_UNSAFE (&hashmap_tx_unsafe_ops)

#ifdef __cplusplus
}
#endif

#endif /* MAP_HASHMAP_TX_UNSAFE_H */
