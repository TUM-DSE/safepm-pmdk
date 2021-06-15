#ifndef LIBPMEMOBJ_ASAN_WRAPPERS_H
#define LIBPMEMOBJ_ASAN_WRAPPERS_H

#include "asan.h"
#include <libpmemobj/types.h>
#include "tx.h"

POBJ_LAYOUT_BEGIN(libpmemobj_secure); // The exact layout name here does not matter
POBJ_LAYOUT_ROOT(libpmemobj_secure, struct pmemobj_asan_root);
POBJ_LAYOUT_TOID(libpmemobj_secure, struct pmemobj_asan_shadowmem);
POBJ_LAYOUT_TOID(libpmemobj_secure, struct pmemobj_asan_end); // A special type marking the highest type number used by SPMO
POBJ_LAYOUT_END(libpmemobj_secure);

struct pmemobj_asan_shadowmem {}; // A dummy type, whose type number is used for the shadow memory.

struct pmemobj_asan_root {
	PMEMoid shadow_mem;
	uint64_t pool_size;
	PMEMoid real_root;
	uint64_t real_root_size;
};

struct pmemobj_asan_end {};

#endif
