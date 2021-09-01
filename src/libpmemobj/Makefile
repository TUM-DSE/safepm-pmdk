# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2014-2020, Intel Corporation

#
# src/libpmemobj/Makefile -- Makefile for libpmemobj
#

LIBRARY_NAME = pmemobj
LIBRARY_SO_VERSION = 1
LIBRARY_VERSION = 0.0

include ../core/pmemcore.inc
include ../common/pmemcommon.inc

SOURCE +=\
	alloc_class.c\
	bucket.c\
	container_ravl.c\
	container_seglists.c\
	critnib.c\
	ctl_debug.o\
	heap.c\
	lane.c\
	libpmemobj.c\
	list.c\
	memblock.c\
	memops.c\
	obj.c\
	palloc.c\
	pmalloc.c\
	recycler.c\
	sync.c\
	tx.c\
	stats.c\
	ulog.c\
	asan_overmap.c\
	asan_wrappers.c\
	asan_verifier.c

include ../Makefile.inc

CFLAGS += -DUSE_LIBDL -D_PMEMOBJ_INTRNL $(LIBNDCTL_CFLAGS)

LIBS += -pthread -lpmem $(LIBDL) $(LIBNDCTL_LIBS)

ifeq ($(PMASAN_TRACK_SPACE_USAGE), 1)
CFLAGS += -DPMASAN_TRACK_SPACE_USAGE=$(PMASAN_TRACK_SPACE_USAGE)
endif

# Compile asan_verifier.c with asan
# The snippet below is copied from ../Makefile.inc, and modified to compile asan_verifier.c with ASan
$(objdir)/asan_verifier.o: asan_verifier.c $(MAKEFILE_DEPS)
	$(call check-cstyle, $<)
	@mkdir -p $(objdir)/.deps
	$(CC) -MD -c -o $@ $(CFLAGS) $(INCS) -fPIC $(call coverage-path, $<) -fsanitize=address
	$(call check-os, $@, $<)
	$(create-deps)
