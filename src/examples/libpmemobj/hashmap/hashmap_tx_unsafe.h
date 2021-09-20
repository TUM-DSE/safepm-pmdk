// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2017, Intel Corporation */
#ifndef HASHMAP_TX_UNSAFE_H
#define HASHMAP_TX_UNSAFE_H

#include <stddef.h>
#include <stdint.h>
#include <hashmap.h>
#include <libpmemobj.h>

#ifndef HASHMAP_TX_UNSAFE_TYPE_OFFSET
#define HASHMAP_TX_UNSAFE_TYPE_OFFSET 1111
#endif

#if defined(__clang__) || defined (__GNUC__)
# define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

struct hashmap_tx;
TOID_DECLARE(struct hashmap_tx, HASHMAP_TX_UNSAFE_TYPE_OFFSET + 0);

int hm_tx_unsafe_check(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap);
int hm_tx_unsafe_create(PMEMobjpool *pop, TOID(struct hashmap_tx) *map, void *arg);
int hm_tx_unsafe_init(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap);
int hm_tx_unsafe_insert(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap,
		uint64_t key, PMEMoid value);
PMEMoid hm_tx_unsafe_remove(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap,
		uint64_t key);
PMEMoid hm_tx_unsafe_get(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap,
		uint64_t key);
int hm_tx_unsafe_lookup(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap,
		uint64_t key);
int hm_tx_unsafe_foreach(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap,
	int (*cb)(uint64_t key, PMEMoid value, void *arg), void *arg);
size_t hm_tx_unsafe_count(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap);
int hm_tx_unsafe_cmd(PMEMobjpool *pop, TOID(struct hashmap_tx) hashmap,
		unsigned cmd, uint64_t arg);

#endif /* HASHMAP_TX_UNSAFE_H */
