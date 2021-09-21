// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2019, Intel Corporation */

/*
 * tx.h -- internal definitions for transactions
 */

#ifndef LIBPMEMOBJ_INTERNAL_TX_H
#define LIBPMEMOBJ_INTERNAL_TX_H 1

#include <stdint.h>
#include "obj.h"
#include "ulog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TX_DEFAULT_RANGE_CACHE_SIZE (1 << 15)
#define TX_DEFAULT_RANGE_CACHE_THRESHOLD (1 << 12)

#define TX_RANGE_MASK (8ULL - 1)
#define TX_RANGE_MASK_LEGACY (32ULL - 1)

#define TX_ALIGN_SIZE(s, amask) (((s) + (amask)) & ~(amask))

#define TX_SNAPSHOT_LOG_ENTRY_ALIGNMENT CACHELINE_SIZE
#define TX_SNAPSHOT_LOG_BUFFER_OVERHEAD sizeof(struct ulog)
#define TX_SNAPSHOT_LOG_ENTRY_OVERHEAD sizeof(struct ulog_entry_buf)

#define TX_INTENT_LOG_BUFFER_ALIGNMENT CACHELINE_SIZE
#define TX_INTENT_LOG_BUFFER_OVERHEAD sizeof(struct ulog)
#define TX_INTENT_LOG_ENTRY_OVERHEAD sizeof(struct ulog_entry_val)

struct tx_parameters {
	size_t cache_size;
};

/*
 * Returns the current transaction's pool handle, NULL if not within
 * a transaction.
 */
PMEMobjpool *tx_get_pop(void);

void tx_ctl_register(PMEMobjpool *pop);

struct tx_parameters *tx_params_new(void);
void tx_params_delete(struct tx_parameters *tx_params);

PMEMoid
pmemobj_tx_alloc_no_asan(size_t size, uint64_t type_num);
PMEMoid
pmemobj_tx_xalloc_no_asan(size_t size, uint64_t type_num, uint64_t flags);
PMEMoid
pmemobj_tx_zalloc_no_asan(size_t size, uint64_t type_num);
int
pmemobj_tx_free_no_asan(PMEMoid oid);
int
pmemobj_tx_xfree_no_asan(PMEMoid oid, uint64_t flags);
PMEMoid
pmemobj_tx_realloc_no_asan(PMEMoid oid, size_t size, uint64_t type_num);
PMEMoid
pmemobj_tx_zrealloc_no_asan(PMEMoid oid, size_t size, uint64_t type_num);
int
pmemobj_tx_add_range_no_asan(PMEMoid oid, uint64_t hoff, size_t size);
int
pmemobj_tx_xadd_range_no_asan(PMEMoid oid, uint64_t hoff, size_t size, uint64_t flags);
int
pmemobj_tx_add_range_direct_no_asan(const void *ptr, size_t size);
int
pmemobj_tx_xadd_range_direct_no_asan(const void *ptr, size_t size, uint64_t flags);

#ifdef __cplusplus
}
#endif

#endif
