/*
 * minimemory.h
 *
 *  Created on: Apr 15, 2018
 *      Author: jock
 */

//----------------------------------------------------------------
// Statically-allocated memory manager
//
// by Eli Bendersky (eliben@gmail.com)
//
// This code is in the public domain.
//----------------------------------------------------------------

#ifndef MINIMEMORY_MINIMEMORY_H_
#define MINIMEMORY_MINIMEMORY_H_
//git test
#ifdef __MBED__
#	include "../mbed.h"
#else
#	include <stdint.h>
#endif

#include <stdint.h>
//
// Memory manager: dynamically allocates memory from
// a fixed pool that is allocated statically at link-time.
//
// Usage: after calling memmgr_init() in your
// initialization routine, just use memmgr_alloc() instead
// of malloc() and memmgr_free() instead of free().
// Naturally, you can use the preprocessor to define
// malloc() and free() as aliases to memmgr_alloc() and
// memmgr_free(). This way the manager will be a drop-in
// replacement for the standard C library allocators, and can
// be useful for debugging memory allocation problems and
// leaks.
//
// Preprocessor flags you can define to customize the
// memory manager:
//
// DEBUG_MEMMGR_FATAL
//    Allow printing out a message when allocations fail
//
// DEBUG_MEMMGR_SUPPORT_STATS
//    Allow printing out of stats in function
//    memmgr_print_stats When this is disabled,
//    memmgr_print_stats does nothing.
//
// Note that in production code on an embedded system
// you'll probably want to keep those undefined, because
// they cause printf to be called.
//
// POOL_SIZE
//    Size of the pool for new allocations. This is
//    effectively the heap size of the application, and can
//    be changed in accordance with the available memory
//    resources.
//
// MIN_POOL_ALLOC_QUANTAS
//    Internally, the memory manager allocates memory in
//    quantas roughly the size of two uint32_t objects. To
//    minimize pool fragmentation in case of multiple allocations
//    and deallocations, it is advisable to not allocate
//    blocks that are too small.
//    This flag sets the minimal ammount of quantas for
//    an allocation. If the size of a uint32_t is 4 and you
//    set this flag to 16, the minimal size of an allocation
//    will be 4 * 2 * 16 = 128 bytes
//    If you have a lot of small allocations, keep this value
//    low to conserve memory. If you have mostly large
//    allocations, it is best to make it higher, to avoid
//    fragmentation.
//
// Notes:
// 1. This memory manager is *not thread safe*. Use it only
//    for single thread/task applications.
//

#define MMP_MIN_POOL_ALLOC_QUANTAS 16

typedef uint32_t Align;

union MmpHeaderUnion
{
    struct
    {
        // Pointer to the next block in the free list
        //
        union MmpHeaderUnion* next;

        // Size of the block (in quantas of sizeof(mem_header_t))
        //
        uint32_t size;
    } s;

    // Used to align headers in memory to a boundary
    //
    Align align_dummy;
};

typedef union MmpHeaderUnion MmpHeader;

typedef struct {
		// Initial empty list
		//
		MmpHeader base;

		// Start of free list
		//
		MmpHeader* freep;

		// Static pool for new allocations
		//
		uint32_t poolSize;
		uint8_t *pool;
		uint32_t pool_free_pos;
} MiniMemoryPool;


// Initialize the memory manager. This function should be called
// only once in the beginning of the program.
//
void mmpInit(MiniMemoryPool *appletMem, uint8_t *memForPool, uint32_t pSize);

// 'malloc' clone
//
void* mmpMalloc(MiniMemoryPool *appletMem, uint32_t nbytes);

// 'realloc' clone
//
void* mmpRealloc(MiniMemoryPool *appletMem, void *ptr, uint32_t size);

// 'free' clone
//
void mmpFree(MiniMemoryPool *appletMem, void* ap);

#endif
