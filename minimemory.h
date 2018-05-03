/*
 * minimemory.h
 *
 *  Created on: Apr 15, 2018
 *      Author: jock
 */
#ifndef MINIMEMORY_MINIMEMORY_H_
#define MINIMEMORY_MINIMEMORY_H_

#ifdef __MBED__
#	include "../mbed.h"
#endif
typedef struct {
		bool      initialized;
		uint8_t   *memoryPool;
		uint16_t  poolSize;
		uint16_t  memTop;

		void     *memoryStart;
		void     *lastValidAddress;
} MiniMemoryPool;

void mmpReset(MiniMemoryPool *pool);
void mmpInit(MiniMemoryPool *pool, void *memPool, uint16_t poolSize);
void mmpFree(MiniMemoryPool *pool, void *firstbyte);
void *mmpMalloc(MiniMemoryPool *pool, uint16_t numbytes);
void *mmpRealloc(MiniMemoryPool *pool, void *prr, uint16_t numbytes);




#endif /* MINIMEMORY_MINIMEMORY_H_ */
