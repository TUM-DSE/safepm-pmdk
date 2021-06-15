#include "asan.h"

#include <assert.h>

/*

shadow memory location: 0x7fff7000..0x10007FFF8000
 location in shadow memory: 0x8FFF6E00..0x2008FFF7000
 thus, the address range    0x8fff7000..0x2008fff7000 is marked unaccessible at the paging level
 this ensures that no instrumented code can try to read/write the shadow memory itself
 the range bw. 6E00 to 7000 is not protected, unfortunately, because the unaccessible region has to be 4kb-aligned.
 
 "The malloc call stack is stored in the left redzone (the larger the redzone, the larger the number of frames that can be stored) while the free call stack is stored in the beginning of the memory region itself."
    ->storing the call stack, which is composed of volatile/randomized pointers, is probably not helpful in the context of persistent memory
*/

uint8_t* pmdk_asan_get_shadow_mem_location(void* _p) {
		  uint64_t p = (uint64_t)_p;
		  return (uint8_t*)((p>>3)+0x7fff8000);
}

// Even with no_sanitize, calls to memset get intercepted by ASan,
//  which is uncomfortable with us directly modifying the shadow memory
__attribute__((no_sanitize("address")))
void pmdk_asan_memset(void* start_, uint8_t byt, size_t len) {
	uint8_t* start = start_;
	while (len) {
		*start = byt;
		start++;
		len--;
	}
}

__attribute__((no_sanitize("address")))
void pmdk_asan_memcpy(void* dest_, const void* src_, size_t len) {
        uint8_t* dest = (uint8_t*)dest_;
        const uint8_t* src = (const uint8_t*)src_;
        while (len) {
                *(uint8_t*)dest = *(uint8_t*)src;
                dest++;
                src++;
                len--;
        }
}

// len in bytes
__attribute__((no_sanitize("address")))
void pmdk_asan_mark_mem(void* start, size_t len, uint8_t tag) {
	assert((int8_t)tag <= 0);
	if ((uint64_t)start%8) {
		uint64_t misalignment = (uint64_t)start%8;
		/*uint8_t* shadow_pos = get_shadow_mem_location(start);
		*shadow_pos = tag;*/ // We can only enter this branch during the marking of the right red-zone for non-multiple-of-8 sized objects. In this case, we must no modify this bit of the shadow memory.
		start = (void*)((uint64_t)start+8-misalignment);
		len -= 8-misalignment;
	}
	pmdk_asan_memset(pmdk_asan_get_shadow_mem_location(start), tag, len/8);
	if (len%8) {
		int prot = len%8;
		uint8_t* shadow_pos = pmdk_asan_get_shadow_mem_location((uint8_t*)start+len);
		if (tag)
			*shadow_pos = tag; // We don't really need to check the previous value of *shadow_start here, because pmemobj would not distribute the same 8-byte chunk to multiple objects.
		else
			*shadow_pos = (uint8_t)prot;
	}
}

/*
int
pmemobj_asan_tag_mem_tx(void* ptr, size_t size, uint8_t tag) {
	size_t shadow_modification_size = (size+7)/8 + off%8;
	PMEMoid psm = pmemobj_asan_ptr2psm(ptr);
	uint64_t off = ((uint8_t* )ptr - (uint8_t*)pmemobj_pool_by_ptr(ptr))/8;

	// kartal TODO: instead of adding part of the shadow mem to the transaction as a snapshot,
	//              use a custom ulog operation to save persistent memory
	int ret = pmemobj_tx_add_range(psm, off, shadow_modification_size);
	if (ret) {
		return ret;
	}

	pmemobj_asan_mark_mem(ptr, size, tag);

	return 0;
}
*/
