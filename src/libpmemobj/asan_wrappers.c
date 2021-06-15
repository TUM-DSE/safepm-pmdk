#include "asan_wrappers.h"
#include "asan_overmap.h"
#include "asan.h"
#include "libpmemobj/base.h"
#include "libpmemobj/tx_base.h"
#include "obj.h"
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <assert.h>
#include <set.h>
#include "os.h"

static PMEMoid pmemobj_asan_pool2psm(PMEMobjpool* pool) {
	PMEMoid rootp_ = pmemobj_root_no_asan(pool, sizeof(struct pmemobj_asan_root)); // TODO: A tighter integration with libpmemobj could let us avoid re-retrieving the persistent shadow memory location for each operation
	const struct pmemobj_asan_root* rootp = pmemobj_direct(rootp_);
	return rootp->shadow_mem;
}

static PMEMoid pmemobj_asan_oid2psm(PMEMoid oid) {
	PMEMobjpool* pool = pmemobj_pool_by_oid(oid);
	return pmemobj_asan_pool2psm(pool);
}

/*static PMEMoid pmemobj_asan_ptr2psm(const void* ptr) {
	PMEMobjpool* pool = pmemobj_pool_by_ptr(ptr);
	return pmemobj_asan_pool2psm(pool);
}*/

static PMEMoid alloc_additional_work(PMEMoid orig, size_t size) {
	if (OID_IS_NULL(orig)) {
		return orig;
	}
	uint8_t* direct = (uint8_t*)pmemobj_direct(orig);
	PMEMoid shadow = pmemobj_asan_oid2psm(orig);

	int res = pmemobj_tx_add_range(shadow, orig.off/8, (2*pmdk_asan_RED_ZONE_SIZE+size+7)/8);
	if (res) {
		return OID_NULL;
	}
	
	pmdk_asan_mark_mem(direct, pmdk_asan_RED_ZONE_SIZE, pmdk_asan_LEFT_REDZONE);
	pmdk_asan_mark_mem(direct+pmdk_asan_RED_ZONE_SIZE, size, pmdk_asan_ADDRESSABLE); // In case it was poisoned by an earlier free
	pmdk_asan_mark_mem(direct+pmdk_asan_RED_ZONE_SIZE+size, pmdk_asan_RED_ZONE_SIZE, pmdk_asan_RIGHT_REDZONE);

	orig.off += pmdk_asan_RED_ZONE_SIZE;
	return orig;
}

static void overmap_pool(const char* path, PMEMobjpool* pool) {
	PMEMoid shadow = pmemobj_asan_pool2psm(pool);
	int fd = os_open(path, O_RDWR);
	assert(fd != -1); // TODO: Handle errors gracefully
	uint64_t pool_end = (uint64_t)pool + pool->set->poolsize;
	pmemobj_asan_overmap((void*)pool, (void *)pool_end, fd, (off_t)shadow.off);
	close(fd);
}

static void undo_overmap_pool(PMEMobjpool* pool) {
	PMEMoid roott = pmemobj_root_no_asan(pool, sizeof(struct pmemobj_asan_root));
	const struct pmemobj_asan_root *rootp = pmemobj_direct(roott);
	uint64_t pool_end = (uint64_t)pool + rootp->pool_size;
	pmemobj_asan_undo_overmap((void*)pool, (void *)pool_end);
}

static char* add_layout_prefix(const char* given_layout) {
	size_t given_layout_len = strlen(given_layout);
	const char* prefix = "spmo_";
	const size_t prefix_len = strlen(prefix);
	size_t new_layout_len = given_layout_len + prefix_len;
	char* layout = malloc(new_layout_len + 1);
	snprintf(layout, new_layout_len + 1, "%s%s", prefix, given_layout);
	return layout;
}

PMEMobjpool *pmemobj_open(const char *path, const char *given_layout) {
	PMEMobjpool* pool;
	if (1) {
		char* layout = add_layout_prefix(given_layout);
		pool = pmemobj_open_no_asan(path, layout);
		free(layout);
	}

	if (pool == NULL)
		return NULL;
	overmap_pool(path, pool);
	
	return pool;
}
PMEMobjpool *pmemobj_create(const char *path, const char *real_layout, size_t poolsize, mode_t mode) {
	if (poolsize%(4096*8)) // Poolsize needs to be 8*4kb-padded because the shadow memory needs to be 4kb-padded (for marking the red-zone)
		poolsize += (4096*8) - poolsize%(4096*8);
	PMEMobjpool* pool;
	if (1) {
		char* layout = add_layout_prefix(real_layout);
		pool = pmemobj_create_no_asan(path, layout, poolsize, mode);
		free(layout);
	}
	if (pool == NULL)
		return NULL;
	PMEMoid roott = pmemobj_root_no_asan(pool, sizeof(struct pmemobj_asan_root));
	assert(! OID_IS_NULL(roott));

	struct pmemobj_asan_root* rootp = pmemobj_direct(roott);
	rootp->pool_size = poolsize;
	rootp->real_root_size = 0;
	size_t shadow_size = poolsize/8; // The shadow memory encompasses information about the whole pool: including the pmdk header and the shadow memory itself
	// Allocate and zero-initialize the persistent shadow memory
	
	assert(0 == pmemobj_alloc_no_asan(pool, &rootp->shadow_mem, shadow_size+4096, TOID_TYPE_NUM(struct pmemobj_asan_shadowmem), NULL, NULL)); // The offset of the shadow memory needs to be 4k aligned (because we want to be able to map it independently of the rest of the pool), so allocate some additional space
	if (rootp->shadow_mem.off % (4096)) // Make sure the shadow mem offset is 4kb aligned. We don't need to store the original address returned by POBJ_ZALLOC because we won't free the shadow memory
		rootp->shadow_mem.off += 4096 - rootp->shadow_mem.off%(4096);

	pmemobj_persist(pool, rootp, sizeof(struct pmemobj_asan_root));

	// Overmap the persistent shadow memory on top of the (volatile) shadow memory created by ASan
	overmap_pool(path, pool);
	
	uint8_t* vmem_shadow_mem_start = pmemobj_direct(rootp->shadow_mem);

	// Mark eerything until the heap "metadata"
	pmemobj_memset_persist(pool, vmem_shadow_mem_start, pmdk_asan_METADATA, pool->heap_offset/8);

	// Mark the entire heap "freed"
	pmemobj_memset_persist(pool, vmem_shadow_mem_start + pool->heap_offset/8, pmdk_asan_FREED, pool->heap_size/8);
	
	// Mark the red zone within the persistent shadow mem
	// The red zone corresponding to the volatile persistent memory range is marked non-accessible on a page permission level, because filling the red zone with -1 would allocate physical memory.
	// We need not resort to such a trick, as we allocate all persistent shadow memory during pool creation.
	pmemobj_memset_persist(pool, vmem_shadow_mem_start + rootp->shadow_mem.off/8, pmdk_asan_INTERNAL, shadow_size/8); // Note that because of the overmapping, the change will be mirrored to the overmapped shadow memory.
	
	return pool;
}

void pmemobj_close(PMEMobjpool *pop) {
	undo_overmap_pool(pop);
	pmemobj_close_no_asan(pop);
}

PMEMoid pmemobj_root(PMEMobjpool *pool, size_t size) {
	PMEMoid roott = pmemobj_root_no_asan(pool, sizeof(struct pmemobj_asan_root));
	assert( ! OID_IS_NULL(roott) );

	struct pmemobj_asan_root* rootp = pmemobj_direct(roott);
	if (OID_IS_NULL(rootp->real_root)) {
		PMEMoid real_root;
		TX_BEGIN(pool) {
			real_root = pmemobj_tx_alloc(size, TOID_TYPE_NUM(struct pmemobj_asan_end)); // Do an asan-aware allocation here

			pmemobj_tx_add_range_direct(&rootp->real_root, 24);

			rootp->real_root = real_root;
			rootp->real_root_size = size;
		} TX_ONABORT {
			return OID_NULL;
		} TX_END
		
		return real_root;
	}
	else {
		// TODO: Remove this condition once we implement realloc
		assert(rootp->real_root_size >= size);
		return rootp->real_root;
	}
}

//PMEMoid spmemobj_root_construct(PMEMobjpool *pop, size_t size, pmemobj_constr constructor, void *arg);

size_t pmemobj_root_size(PMEMobjpool *pool) {
	PMEMoid roott = pmemobj_root_no_asan(pool, sizeof(struct pmemobj_asan_root));
	if (OID_IS_NULL(roott))
		return 0;

	struct pmemobj_asan_root* rootp = pmemobj_direct(roott);
	return rootp->real_root_size;
}

PMEMoid pmemobj_tx_alloc(size_t size, uint64_t type_num) {
	//TODO: A custom allocator could save us one redzone per object.
	PMEMoid orig = pmemobj_tx_alloc_no_asan(size+2*pmdk_asan_RED_ZONE_SIZE, type_num+TOID_TYPE_NUM(struct pmemobj_asan_end));
	return alloc_additional_work(orig, size);
}

int pmemobj_tx_free(PMEMoid oid) {
	uint8_t* shadow_object_start = pmdk_asan_get_shadow_mem_location(pmemobj_direct(oid));
	assert((int8_t)(*shadow_object_start) >= 0 && "Invalid free");
	assert(*(shadow_object_start-1) == pmdk_asan_LEFT_REDZONE && "Invalid free");
	PMEMoid redzone_start={.pool_uuid_lo = oid.pool_uuid_lo, .off = oid.off - pmdk_asan_RED_ZONE_SIZE};
	int res;
	if ((res = pmemobj_tx_free_no_asan(redzone_start))) // TODO: Quarantine the region to provide additional temporal safety
		return res;

	uint64_t size = pmemobj_alloc_usable_size_no_asan(redzone_start);
	PMEMoid shadow_oid = pmemobj_asan_oid2psm(oid);
	if ((res = pmemobj_tx_add_range(shadow_oid, redzone_start.off/8, size/8)))
		return res;
	pmdk_asan_mark_mem(pmemobj_direct(redzone_start), size, pmdk_asan_FREED);

	return 0;
}
PMEMoid pmemobj_tx_zalloc(size_t size, uint64_t type_num) {
	PMEMoid user = pmemobj_tx_alloc(size, type_num);
	if (OID_IS_NULL(user))
		return user;

	memset(pmemobj_direct(user), 0, size);
	return user;
}
size_t
pmemobj_alloc_usable_size(PMEMoid oid) {
	if (OID_IS_NULL(oid))
		return 0;

	oid.off -= pmdk_asan_RED_ZONE_SIZE;
	size_t res = pmemobj_alloc_usable_size_no_asan(oid);
	if (res == 0)
		return 0;
	return res - 2*pmdk_asan_RED_ZONE_SIZE;
}
uint64_t
pmemobj_type_num(PMEMoid oid) {
	ASSERT(!OID_IS_NULL(oid));
	oid.off -= pmdk_asan_RED_ZONE_SIZE;
	return pmemobj_type_num_no_asan(oid) - TOID_TYPE_NUM(struct pmemobj_asan_end);
}
int pmemobj_alloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size,
    uint64_t type_num, pmemobj_constr constructor, void *arg) {
	if (size == 0) {
		ERR("allocation with size 0");
		errno = EINVAL;
		return -1;
	}

	volatile int cancelled = 0;
	TX_BEGIN(pop) {
		PMEMoid oid = pmemobj_tx_alloc(size, type_num); // Note that this is an asan-aware allocation
		if (oidp != NULL) {
			if ((uint8_t*)oidp >= (uint8_t*)pop + pop->heap_offset &&
				(uint8_t*)oidp < (uint8_t*)pop + pop->heap_offset + pop->heap_size) {
				pmemobj_tx_add_range_direct(oidp, sizeof(PMEMoid));
			}
			*oidp = oid;
		}
		if (constructor != NULL)
			if (constructor(pop, pmemobj_direct(oid), arg) != 0)
				pmemobj_tx_abort(ECANCELED);
	} TX_ONABORT {
		cancelled = 1;
	} TX_END
	if (cancelled)
		return -1;
	return 0;
}
static int zalloc_zeroer(PMEMobjpool *pop, void *ptr, void *arg) {
	TX_MEMSET(ptr, 0, (size_t)arg);
	return 0;
}
int pmemobj_zalloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size,
    uint64_t type_num) {
	return pmemobj_alloc(pop, oidp, size, type_num, zalloc_zeroer, (void*)size);
}
void pmemobj_free(PMEMoid *oidp) {
	ASSERTne(oidp, NULL);
	if (OID_IS_NULL(*oidp))
		return ;

	PMEMobjpool* pop = pmemobj_pool_by_oid(*oidp);
	TX_BEGIN(pop) {
		pmemobj_tx_free(*oidp); // Note that this is an asan-aware de-allocation
		if ((uint8_t*)oidp >= (uint8_t*)pop + pop->heap_offset &&
			(uint8_t*)oidp < (uint8_t*)pop + pop->heap_offset + pop->heap_size) {
			pmemobj_tx_add_range_direct(oidp, sizeof(PMEMoid));
		}
		*oidp = OID_NULL;
	} TX_END
}

PMEMoid
pmemobj_first(PMEMobjpool *pop) {
	PMEMoid res = pmemobj_first_no_asan(pop);
	if (OID_IS_NULL(res))
		return res;
	if (pmemobj_type_num_no_asan(res) < TOID_TYPE_NUM(struct pmemobj_asan_end)) {
		res.off += pmdk_asan_RED_ZONE_SIZE; // kartal TODO: Once we backport the redzone-as-object-header approach, we won't need all this pointer juggling
		return pmemobj_next(res);
	}
	res.off += pmdk_asan_RED_ZONE_SIZE;
	return res;
}
PMEMoid
pmemobj_next(PMEMoid oid) {
	oid.off -= pmdk_asan_RED_ZONE_SIZE;
	PMEMoid res = pmemobj_next_no_asan(oid);
	if (OID_IS_NULL(res))
		return res;
	if (pmemobj_type_num_no_asan(res) < TOID_TYPE_NUM(struct pmemobj_asan_end)) {
		res.off += pmdk_asan_RED_ZONE_SIZE;
		return pmemobj_next(res);
	}
	res.off += pmdk_asan_RED_ZONE_SIZE;
	return res;
}

//PMEMoid spmemobj_tx_realloc(PMEMoid oid, size_t size, uint64_t type_num);
//PMEMoid spmemobj_tx_zrealloc(PMEMoid oid, size_t size, uint64_t type_num);
//PMEMoid spmemobj_tx_strdup(const char *s, uint64_t type_num);
//int pmemobj_realloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num);
//int pmemobj_zrealloc(PMEMobjpool *pop, PMEMoid *oidp, size_t size, uint64_t type_num);
//int pmemobj_strdup(PMEMobjpool *pop, PMEMoid *oidp, const char *s, uint64_t type_num);