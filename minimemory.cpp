#include "../mbed.h"
#include "minimemory.h"

static void *expandPool(MiniMemoryPool *pool, uint16_t sz)
{
	if(pool->memTop + sz >= pool->poolSize) {
		return NULL;
	}

	uint16_t ret = pool->memTop;
	pool->memTop += sz;

	char *p = (char *) &pool->memoryPool[ret];
	return (void *) p;
}

void mmpReset(MiniMemoryPool *pool)
{
	mmpInit(pool, pool->memoryPool, pool->poolSize);
}

///////////////////////////////////////////////////////////////////////
// All code from below this comment based on:
//     https://www.ibm.com/developerworks/jp/linux/library/l-memory/
// and may have been modified
///////////////////////////////////////////////////////////////////////

void mmpInit(MiniMemoryPool *pool, uint8_t *memPool, uint16_t poolSize)
{
	pool->memoryPool = memPool;
	pool->poolSize = poolSize;
	pool->memTop = 0;

	// get the last valid address from the OS
	pool->lastValidAddress = expandPool(pool, 0);

	// we don't have any memory to manage yet, so
	// just set the beginning to be last_valid_address
	pool->memoryStart = pool->lastValidAddress;

	// Okay, we're initialized and ready to go
	pool->initialized = true;

	memset(pool->memoryPool, 0, pool->poolSize);
}

// TODO JEM: Condense this into a single 16 bit word with the high bit
// being a flag
typedef struct _memoryControlBlock
{
		bool available;
		uint16_t size;
} MemoryControlBlock;

void mmpFree(MiniMemoryPool *pool, void *firstbyte)
{
	MemoryControlBlock *mcb;

	mcb = (struct memoryControlBlock *) ((uint8_t *) firstbyte
			- sizeof(struct memoryControlBlock));
	mcb->available = true;

	return;
}

void *mmpMalloc(MiniMemoryPool *pool, uint16_t numbytes)
{
	// Holds where we are looking in memory
	void *currentLocation;

	// This is the same as current_location, but cast to a * memory_control_block
	MemoryControlBlock *currentLocationMcb;

	// This is the memory location we will return.
	//It will * be set to 0 until we find something suitable
	void *memoryLocation;

	// The memory we search for has to include the memory
	// control block, but the user of malloc doesn't need
	// to know this, so we'll just add it in for them.
	numbytes = numbytes + sizeof(struct memoryControlBlock);

	// Set memory_location to 0 until we find a suitable * location
	memoryLocation = 0;

	// Begin searching at the start of managed memory
	currentLocation = pool->memoryStart;

	// Keep going until we have searched all allocated space
	while(currentLocation != pool->lastValidAddress) {
		// current_location and current_location_mcb point
		// to the same address.  However, current_location_mcb
		// is of the correct type so we can use it as a struct.
		// current_location is a void pointer so we can use it
		// to calculate addresses.
		currentLocationMcb = (struct memoryControlBlock *) currentLocation;

		if(currentLocationMcb->available) {
			if(currentLocationMcb->size >= numbytes) {
				// Woohoo!  We've found an open, * appropriately-size location.
				// It is no longer available
				currentLocationMcb->available = false;

				// We own it
				memoryLocation = currentLocation;

				// Leave the loop
				break;
			}
		}

		// If we made it here, it's because the Current memory
		// block not suitable, move to the next one
		currentLocation = ((uint8_t *) currentLocation)
				+ currentLocationMcb->size;
	}

	// If we still don't have a valid location, we'll
	// have to ask the operating system for more memory
	if(!memoryLocation) {
		// Move the program break numbytes further
		expandPool(pool, numbytes);

		// The new memory will be where the last valid * address left off
		memoryLocation = pool->lastValidAddress;

		// We'll move the last valid address forward * numbytes
		pool->lastValidAddress = ((uint8_t *) pool->lastValidAddress)
				+ numbytes;

		// We need to initialize the mem_control_block
		currentLocationMcb = (MemoryControlBlock *) memoryLocation;
		currentLocationMcb->available = false;
		currentLocationMcb->size = numbytes;
	}

	// Now, no matter what (well, except for error conditions),
	// memory_location has the address of the memory, including
	// the mem_control_block
	// Move the pointer past the mem_control_block
	memoryLocation = ((uint8_t *) memoryLocation)
			+ sizeof(struct memoryControlBlock);

	// Return the pointer
	return memoryLocation;
}

///////////////////////////////////////////////////////////////////////
// End of code based on:
//     https://www.ibm.com/developerworks/jp/linux/library/l-memory/
///////////////////////////////////////////////////////////////////////

void *mmpRealloc(MiniMemoryPool *pool, void *ptr, uint16_t numbytes)
{
	// If the pointer is null, we are just a malloc
	if(!ptr)
	{
		void *ret = mmpMalloc(pool, numbytes);
		return ret;
	}

	// if the size is zero, we are just a free
	if(numbytes == 0) {
		mmpFree(pool, ptr);
		return NULL;
	}


	MemoryControlBlock *mcb;

	mcb = (struct memoryControlBlock *) ((uint8_t *) ptr
			- sizeof(struct memoryControlBlock));

	// if they are requesting a size smaller than their current size,
	// then leave.
	// TODO check if the delta is above a certain threshold and recover space
	if(numbytes <= mcb->size) {
		return ptr;
	}

	// With all that out of the way, see if we can expand the pool in place
	// or if we have to allocate and copy

	uint16_t oldLen = mcb->size;
	uint16_t allocDelta = numbytes = oldLen;
	uint8_t *allocTop = ((uint8_t *) ptr) + (oldLen - 1);

	bool canExpand = allocTop == pool->lastValidAddress;

	if(canExpand) {
		void *tPtr = expandPool(pool, allocDelta);
		if(tPtr == NULL) {
			return NULL;
		}

		pool->lastValidAddress = expandPool(pool, 0);

		return ptr;
	} else {
		void *ret = mmpMalloc(pool, numbytes);
		if(ret) {
			memcpy(ret, ptr, oldLen);
			mmpFree(pool, ptr);
		}
		return ret;
	}
}
