/*
 * minimemory.h
 *
 *  Created on: Apr 15, 2018
 *      Author: jock
 */
#ifndef MINIMEMORY_MINIMEMORY_H_
#define MINIMEMORY_MINIMEMORY_H_

#include "../mbed.h"

typedef struct {
		bool      initialized;
		uint8_t   *memoryPool;
		uint16_t  poolSize;
		uint16_t  memTop;

		void     *managed_memory_start;
		void     *last_valid_address;
} MiniMemoryPool;

void mmpReset(MiniMemoryPool *pool);
void miniMemoryInit(MiniMemoryPool *pool, void *memPool, uint16_t poolSize);
void mmpFree(MiniMemoryPool *pool, void *firstbyte);
void *mmpMalloc(MiniMemoryPool *pool, uint16_t numbytes);




#endif /* MINIMEMORY_MINIMEMORY_H_ */
