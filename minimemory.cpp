#include "../mbed.h"
#include "minimemory.h"

static void *expandPool(MiniMemoryPool *pool, uint16_t sz)
{
	if(pool->memTop + sz >= pool->poolSize) {
		return NULL;
	}

	uint16_t ret = pool->memTop;
	pool->memTop += sz;

	char *p = (char *)&pool->memoryPool[ret];
	return (void *) p;
}


void mmpReset(MiniMemoryPool *pool)
{
	pool->memTop = 0;
}


///////////////////////////////////////////////////////////////////////
// All code from below this comment is from:
//     https://www.ibm.com/developerworks/jp/linux/library/l-memory/
///////////////////////////////////////////////////////////////////////



void mmpInit(MiniMemoryPool *pool, uint8_t *memPool, uint16_t poolSize) {
	pool->memoryPool = memPool;
	pool->poolSize = poolSize;
	pool->memTop = 0;
	/* grab the last valid address from the OS*/
	pool->last_valid_address = expandPool(pool, 0);

	/* we don't have any memory to manage yet, so
	*just set the beginning to be last_valid_address */
	pool->managed_memory_start = pool->last_valid_address;

	/* Okay, we're initialized and ready to go */
	pool->initialized = true;
}

// TODO JEM: Condense this into a single 16 bit word with the high bit
// being a flag
struct mem_control_block {
  bool is_available;
  uint16_t size;
};

void mmpFree(MiniMemoryPool *pool, void *firstbyte)
{
  struct mem_control_block *mcb;

  mcb = (struct mem_control_block *) (firstbyte - sizeof(struct mem_control_block));
  mcb->is_available = true;

  return;
}

void *mmpMalloc(MiniMemoryPool *pool, uint16_t numbytes) {
	// Holds where we are looking in memory
	void *current_location;

	// This is the same as current_location, but cast to a * memory_control_block
	struct mem_control_block *current_location_mcb;

	// This is the memory location we will return.
	//It will * be set to 0 until we find something suitable
	void *memory_location;

	// Initialize if we haven't already done so
//	if (!pool->initialized) {
//		miniMemoryInit(pool);
//	}

	// The memory we search for has to include the memory
	// control block, but the user of malloc doesn't need
	// to know this, so we'll just add it in for them.
	numbytes = numbytes + sizeof(struct mem_control_block);

	// Set memory_location to 0 until we find a suitable * location
	memory_location = 0;

	// Begin searching at the start of managed memory
	current_location = pool->managed_memory_start;

	// Keep going until we have searched all allocated space
	while (current_location != pool->last_valid_address) {
		// current_location and current_location_mcb point
		// to the same address.  However, current_location_mcb
		// is of the correct type so we can use it as a struct.
		// current_location is a void pointer so we can use it
		// to calculate addresses.
		current_location_mcb = (struct mem_control_block *)current_location;

		if (current_location_mcb->is_available) {
			if (current_location_mcb->size >= numbytes) {
				// Woohoo!  We've found an open, * appropriately-size location.
				// It is no longer available
				current_location_mcb->is_available = false;

				// We own it
				memory_location = current_location;

				// Leave the loop
				break;
			}
		}

		// If we made it here, it's because the Current memory
		// block not suitable, move to the next one
		current_location = current_location + current_location_mcb->size;
	}

	// If we still don't have a valid location, we'll
	// have to ask the operating system for more memory
	if (!memory_location) {
		// Move the program break numbytes further
		expandPool(pool, numbytes);

		// The new memory will be where the last valid * address left off
		memory_location = pool->last_valid_address;

		// We'll move the last valid address forward * numbytes
		pool->last_valid_address = pool->last_valid_address + numbytes;

		// We need to initialize the mem_control_block
		current_location_mcb = (mem_control_block *)memory_location;
		current_location_mcb->is_available = false;
		current_location_mcb->size = numbytes;
	}

	// Now, no matter what (well, except for error conditions),
	// memory_location has the address of the memory, including
	// the mem_control_block */
	// Move the pointer past the mem_control_block
	memory_location = memory_location + sizeof(struct mem_control_block);

	// Return the pointer
	return memory_location;
}
