#ifndef PMEMOBJ_ASAN_COMMON_H
#define PMEMOBJ_ASAN_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <libpmemobj/types.h>
#include "tx.h"

#define pmemobj_asan_ADDRESSABLE 0
#define pmemobj_asan_LEFT_REDZONE 0xFA
#define pmemobj_asan_RIGHT_REDZONE 0xFB
#define pmemobj_asan_FREED 0xFD
#define pmemobj_asan_INTERNAL 0xFE

#define pmemobj_asan_RED_ZONE_SIZE 128

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

uint8_t* pmemobj_asan_get_shadow_mem_location(void* _p);

void pmemobj_asan_memset(uint8_t* start, uint8_t byt, size_t len);
void pmemobj_asan_memcpy(void* dest, const void* src, size_t len);

void pmemobj_asan_mark_mem(void* start, size_t len, uint8_t tag);

//int pmemobj_asan_tag_mem_tx(void* ptr, size_t size, uint8_t tag);

#endif
