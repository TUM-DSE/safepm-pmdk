#include "asan_verifier.h"

#include <stdint.h>

/*

Unlike the rest of PMDK, this file is compiled with ASan.
See ./Makefile

*/

void pmemobj_asan_verify_range_addressable(void* ptr_, size_t size) {
	uint8_t* ptr = (uint8_t*)ptr_;
	while (size > 0 && ((uint64_t)ptr%8)) {
		*(volatile uint8_t*)ptr; // This forces a memory access, which in turn triggers an asan check
		ptr++;
		size--;
	}
	while (size >= 8) {
		*(volatile uint64_t*)ptr;
		ptr+=8;
		size-=8;
	}
	while (size > 0) {
		*(volatile uint8_t*)ptr; // This forces a memory access, which in turn triggers an asan check
		ptr++;
		size--;
	}
}
