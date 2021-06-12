#ifndef PMEMOBJ_ASAN_OVERMAP_H
#define PMEMOBJ_ASAN_OVERMAP_H

#include <sys/types.h>

void pmemobj_asan_overmap(void* start, void* end, int new_shadow_fd, off_t fd_offset);
void pmemobj_asan_undo_overmap(void* start, void* end);

#endif
