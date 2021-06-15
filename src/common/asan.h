#ifndef PMDK_ASAN_COMMON_H
#define PMDK_ASAN_COMMON_H

#include <stdint.h>
#include <stddef.h>

#define pmdk_asan_ADDRESSABLE 0
#define pmdk_asan_LEFT_REDZONE 0xFA
#define pmdk_asan_RIGHT_REDZONE 0xFB
#define pmdk_asan_FREED 0xFD
#define pmdk_asan_INTERNAL 0xFE
#define pmdk_asan_METADATA pmdk_asan_INTERNAL // kartal TODO: Is this correct? What values does ASan use to represent metadata?

#define pmdk_asan_RED_ZONE_SIZE 128

uint8_t* pmdk_asan_get_shadow_mem_location(void* _p);

void pmdk_asan_memset(void* start, uint8_t byt, size_t len);
void pmdk_asan_memcpy(void* dest, const void* src, size_t len);

void pmdk_asan_mark_mem(void* start, size_t len, uint8_t tag);

//int pmemobj_asan_tag_mem_tx(void* ptr, size_t size, uint8_t tag);

#endif
