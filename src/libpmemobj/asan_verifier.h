#ifndef PMEMOBJ_ASAN_VERIFIER_H
#define PMEMOBJ_ASAN_VERIFIER_H

#include <stdlib.h>

void pmemobj_asan_verify_range_addressable(void* ptr_, size_t size);

#endif
