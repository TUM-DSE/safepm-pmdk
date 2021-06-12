#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

#include "asan.h"
#include "asan_overmap.h"

// *start* and *end* need to be 4kb*8 aligned
void pmemobj_asan_overmap(void* start, void* end, int new_shadow_fd, off_t fd_offset)
{
    int prot = PROT_READ | PROT_WRITE;
    const uint64_t size = ((uint64_t)end - (uint64_t)start)/8;
    uint8_t* shadow_start = pmemobj_asan_get_shadow_mem_location((void*)start);
    assert((uint64_t)shadow_start % 4096 == 0);
    assert((uint64_t)pmemobj_asan_get_shadow_mem_location((void*)end) % 4096 == 0);
	//std::cout << "Overmapping the address range " << (void*)shadow_start << " ... " << (void*)(shadow_start+size) << std::endl;
    void* res = mmap((void*)shadow_start, size, prot, MAP_SHARED | MAP_FIXED, new_shadow_fd, fd_offset);
    assert(res == (void*)shadow_start);
}

void pmemobj_asan_undo_overmap(void* start, void* end)
{
    int prot = PROT_READ | PROT_WRITE;
    const uint64_t size = ((uint64_t)end - (uint64_t)start)/8;
    uint8_t* shadow_start = pmemobj_asan_get_shadow_mem_location((void*)start);
    assert((uint64_t)shadow_start % 4096 == 0);
    assert((uint64_t)pmemobj_asan_get_shadow_mem_location((void*)end) % 4096 == 0);
    void* res = mmap((void*)shadow_start, size, prot, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    assert(res == (void*)shadow_start);
}
