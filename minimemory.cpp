#ifdef __MBED__
#	include "../mbed.h"
#else
#	include <stdio.h>
#	include <stdlib.h>
#	include <string.h>
#endif

#include "minimemory.h"
//git test
// MEMMGR_H//----------------------------------------------------------------
// Statically-allocated memory manager
//
// by Eli Bendersky (eliben@gmail.com)
//
// This code is in the public domain.
//----------------------------------------------------------------
#include <stdio.h>
//#include "memmgr.h"


void mmpInit(MiniMemoryPool *appletMem, uint8_t *memForPool, uint32_t pSize)
{
	appletMem->poolSize = pSize;
	appletMem->pool = memForPool;

	appletMem->base.s.next = 0;
	appletMem->base.s.size = 0;
	appletMem->freep = 0;
	appletMem->pool_free_pos = 0;
}


//void memmgr_print_stats()
//{
//    #ifdef DEBUG_MEMMGR_SUPPORT_STATS
//    mem_header_t* p;
//
//    printf("------ Memory manager stats ------\n\n");
//    printf(    "Pool: free_pos = %lu (%lu bytes left)\n\n",
//    		appletMem->pool_free_pos, POOL_SIZE - appletMem->pool_free_pos);
//
//    p = (mem_header_t*) appletMem->pool;
//
//    while (p < (mem_header_t*) (appletMem->pool + appletMem->pool_free_pos))
//    {
//        printf(    "  * Addr: %p; Size: %8lu\n",
//                p, p->s.size);
//
//        p += p->s.size;
//    }
//
//    printf("\nFree list:\n\n");
//
//    if (appletMem->freep)
//    {
//        p = appletMem->freep;
//
//        while (1)
//        {
//            printf(    "  * Addr: %p; Size: %8lu; Next: %p\n",
//                    p, p->s.size, p->s.next);
//
//            p = p->s.next;
//
//            if (p == appletMem->freep)
//                break;
//        }
//    }
//    else
//    {
//        printf("Empty\n");
//    }
//
//    printf("\n");
//    #endif // DEBUG_MEMMGR_SUPPORT_STATS
//}

static MmpHeader* get_mem_from_pool(MiniMemoryPool *appletMem, uint32_t nquantas)
{
    uint32_t total_req_size;

    MmpHeader* h;

    if (nquantas < MMP_MIN_POOL_ALLOC_QUANTAS)
        nquantas = MMP_MIN_POOL_ALLOC_QUANTAS;

    total_req_size = nquantas * sizeof(MmpHeader);

    if (appletMem->pool_free_pos + total_req_size <= appletMem->poolSize)
    {
        h = (MmpHeader*) (appletMem->pool + appletMem->pool_free_pos);
        h->s.size = nquantas;
        mmpFree(appletMem, (void*) (h + 1));
        appletMem->pool_free_pos += total_req_size;
    }
    else
    {
        return 0;
    }

    return appletMem->freep;
}


// Allocations are done in 'quantas' of header size.
// The search for a free block of adequate size begins at the point 'freep'
// where the last block was found.
// If a too-big block is found, it is split and the tail is returned (this
// way the header of the original needs only to have its size adjusted).
// The pointer returned to the user points to the free space within the block,
// which begins one quanta after the header.
//
void* mmpMalloc(MiniMemoryPool *appletMem, uint32_t nbytes)
{
    MmpHeader* p;
    MmpHeader* prevp;

    // Calculate how many quantas are required: we need enough to house all
    // the requested bytes, plus the header. The -1 and +1 are there to make sure
    // that if nbytes is a multiple of nquantas, we don't allocate too much
    //
    uint32_t nquantas = (nbytes + sizeof(MmpHeader) - 1) / sizeof(MmpHeader) + 1;

    // First alloc call, and no free list yet ? Use 'base' for an initial
    // denegerate block of size 0, which points to itself
    //
    if ((prevp = appletMem->freep) == 0)
    {
    	appletMem->base.s.next = appletMem->freep = prevp = &appletMem->base;
    	appletMem->base.s.size = 0;
    }

    for (p = prevp->s.next; ; prevp = p, p = p->s.next)
    {
        // big enough ?
        if (p->s.size >= nquantas)
        {
            // exactly ?
            if (p->s.size == nquantas)
            {
                // just eliminate this block from the free list by pointing
                // its prev's next to its next
                //
                prevp->s.next = p->s.next;
            }
            else // too big
            {
                p->s.size -= nquantas;
                p += p->s.size;
                p->s.size = nquantas;
            }

            appletMem->freep = prevp;
            return (void*) (p + 1);
        }
        // Reached end of free list ?
        // Try to allocate the block from the pool. If that succeeds,
        // get_mem_from_pool adds the new block to the free list and
        // it will be found in the following iterations. If the call
        // to get_mem_from_pool doesn't succeed, we've run out of
        // memory
        //
        else if (p == appletMem->freep)
        {
            if ((p = get_mem_from_pool(appletMem, nquantas)) == 0)
            {
                #ifdef DEBUG_MEMMGR_FATAL
                printf("!! Memory allocation failed !!\n");
                #endif
                return 0;
            }
        }
    }

    return NULL;

}


// Scans the free list, starting at freep, looking the the place to insert the
// free block. This is either between two existing blocks or at the end of the
// list. In any case, if the block being freed is adjacent to either neighbor,
// the adjacent blocks are combined.
//
void mmpFree(MiniMemoryPool *appletMem, void* ap)
{
	if(ap == NULL) {
		return;
	}
    MmpHeader* block;
    MmpHeader* p;

    // acquire pointer to block header
    block = ((MmpHeader*) ap) - 1;

    // Find the correct place to place the block in (the free list is sorted by
    // address, increasing order)
    //
    for (p = appletMem->freep; !(block > p && block < p->s.next); p = p->s.next)
    {
        // Since the free list is circular, there is one link where a
        // higher-addressed block points to a lower-addressed block.
        // This condition checks if the block should be actually
        // inserted between them
        //
        if (p >= p->s.next && (block > p || block < p->s.next))
            break;
    }

    // Try to combine with the higher neighbor
    //
    if (block + block->s.size == p->s.next)
    {
        block->s.size += p->s.next->s.size;
        block->s.next = p->s.next->s.next;
    }
    else
    {
        block->s.next = p->s.next;
    }

    // Try to combine with the lower neighbor
    //
    if (p + p->s.size == block)
    {
        p->s.size += block->s.size;
        p->s.next = block->s.next;
    }
    else
    {
        p->s.next = block;
    }

    appletMem->freep = p;
}


void* mmpRealloc(MiniMemoryPool *appletMem, void *ptr, uint32_t size)
{
	// If the pointer is null, we are just a malloc
	if(ptr == NULL)
	{
		void *ret =mmpMalloc(appletMem, size);
		return ret;
	}

	// if the size is zero, we are just a free
	if(size == 0) {
		mmpFree(appletMem, ptr);
		return NULL;
	}

	void *ret = mmpMalloc(appletMem, size);
	if(ret) {
		memcpy(ret, ptr, size);
		mmpFree(appletMem, ptr);
	}

	return ret;
}
