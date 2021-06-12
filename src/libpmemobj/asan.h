#ifndef PMEMOBJ_ASAN_COMMON_H
#define PMEMOBJ_ASAN_COMMON_H

#include <stdint.h>
#include <stdlib.h>

#define pmemobj_asan_ADDRESSABLE 0
#define pmemobj_asan_LEFT_REDZONE 0xFA
#define pmemobj_asan_RIGHT_REDZONE 0xFB
#define pmemobj_asan_FREED 0xFD
#define pmemobj_asan_INTERNAL 0xFE

#define pmemobj_asan_RED_ZONE_SIZE 128

uint8_t* pmemobj_asan_get_shadow_mem_location(void* _p);

void pmemobj_asan_memset(uint8_t* start, uint8_t byt, size_t len);

void pmemobj_asan_mark_mem(void* start, size_t len, uint8_t tag);

int pmemobj_asan_tag_mem_tx(uint64_t off, size_t size, uint8_t tag);

#endif
