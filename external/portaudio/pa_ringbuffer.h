#ifndef PA_RINGBUFFER_H
#define PA_RINGBUFFER_H
/*
 * $Id$
 * Portable Audio I/O Library
 * Ring Buffer utility.
 *
 * Author: Phil Burk, http://www.softsynth.com
 * modified for SMP safety on OS X by Bjorn Roche.
 * also allowed for const where possible.
 * modified for multiple-byte-sized data elements by Sven Fischer 
 *
 * Note that this is safe only for a single-thread reader
 * and a single-thread writer.
 *
 * This program is distributed with the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/** @file 
 @ingroup common_src
 @brief Single-reader single-writer lock-free ring buffer

 PaUtilRingBuffer is a ring buffer used to transport samples between
 different execution contexts (threads, OS callbacks, interrupt handlers)
 without requiring the use of any locks. This only works when there is
 a single reader and a single writer (ie. one thread or callback writes
 to the ring buffer, another thread or callback reads from it).

 The PaUtilRingBuffer structure manages a ring buffer containing N 
 elements, where N must be a power of two. An element may be any size 
 (specified in bytes).

 The memory area used to store the buffer elements must be allocated by 
 the client prior to calling PaUtil_InitializeRingBuffer() and must outlive
 the use of the ring buffer.
*/

// This version has small additions by randy@madronalabs.com.

#include <iostream>

typedef struct PaUtilRingBuffer
{
    long  bufferSize; /**< Number of elements in FIFO. Power of 2. Set by PaUtil_InitRingBuffer. */
    long  writeIndex; /**< Index of next writable element. Set by PaUtil_AdvanceRingBufferWriteIndex. */
    long  readIndex;  /**< Index of next readable element. Set by PaUtil_AdvanceRingBufferReadIndex. */
    long  bigMask;    /**< Used for wrapping indices with extra bit to distinguish full/empty. */
    long  smallMask;  /**< Used for fitting indices to buffer. */
    long  elementSizeBytes; /**< Number of bytes per element. */
    char  *buffer;    /**< Pointer to the buffer containing the actual data. */
}	PaUtilRingBuffer;

/** Initialize Ring Buffer.

 @param rbuf The ring buffer.

 @param elementSizeBytes The size of a single data element in bytes.

 @param elementCount The number of elements in the buffer (must be power of 2).

 @param dataPtr A pointer to a previously allocated area where the data
 will be maintained.  It must be elementCount*elementSizeBytes long.

 @return -1 if elementCount is not a power of 2, otherwise 0.
*/
long PaUtil_InitializeRingBuffer( PaUtilRingBuffer *rbuf, long elementSizeBytes, long elementCount, void *dataPtr );

/** Clear buffer. Should only be called when buffer is NOT being read.

 @param rbuf The ring buffer.
*/
void PaUtil_FlushRingBuffer( PaUtilRingBuffer *rbuf );

/** Retrieve the number of elements available in the ring buffer for writing.

 @param rbuf The ring buffer.

 @return The number of elements available for writing.
*/
long PaUtil_GetRingBufferWriteAvailable( PaUtilRingBuffer *rbuf );

/** Retrieve the number of elements available in the ring buffer for reading.

 @param rbuf The ring buffer.

 @return The number of elements available for reading.
*/
long PaUtil_GetRingBufferReadAvailable( PaUtilRingBuffer *rbuf );

/** Write data to the ring buffer.

 @param rbuf The ring buffer.

 @param data The address of new data to write to the buffer.

 @param elementCount The number of elements to be written.

 @return The number of elements written.
*/
long PaUtil_WriteRingBuffer( PaUtilRingBuffer *rbuf, const void *data, long elementCount );

// fill buffer with elementCount copies of a constant float value.
//
long PaUtil_WriteRingBufferConstant( PaUtilRingBuffer *rbuf, const float val, long elementCount );

long PaUtil_WriteRingBufferWithOverlapAdd( PaUtilRingBuffer *rbuf, const float *srcData, long elementCount, long overlap );


/** Read data from the ring buffer.
 
 @param rbuf The ring buffer.
 
 @param data The address where the data should be stored.
 
 @param elementCount The number of elements to be read.
 
 @return The number of elements read.
 */
long PaUtil_ReadRingBuffer( PaUtilRingBuffer *rbuf, void *data, long elementCount );

// quick hack by randy
// TODO nicer abstraction for overlap
long PaUtil_ReadRingBufferWithOverlap( PaUtilRingBuffer *rbuf, void *data, long elementCount, long overlap );


/** Get address of region(s) to which we can write data.

 @param rbuf The ring buffer.

 @param elementCount The number of elements desired.

 @param dataPtr1 The address where the first (or only) region pointer will be
 stored.

 @param sizePtr1 The address where the first (or only) region length will be
 stored.

 @param dataPtr2 The address where the second region pointer will be stored if
 the first region is too small to satisfy elementCount.

 @param sizePtr2 The address where the second region length will be stored if
 the first region is too small to satisfy elementCount.

 @return The room available to be written or elementCount, whichever is smaller.
*/
long PaUtil_GetRingBufferWriteRegions( PaUtilRingBuffer *rbuf, long elementCount,
                                       void **dataPtr1, long *sizePtr1,
                                       void **dataPtr2, long *sizePtr2 );

/** Advance the write index to the next location to be written.

 @param rbuf The ring buffer.

 @param elementCount The number of elements to advance.

 @return The new position.
*/
long PaUtil_AdvanceRingBufferWriteIndex( PaUtilRingBuffer *rbuf, long elementCount );

/** Get address of region(s) from which we can write data.

 @param rbuf The ring buffer.

 @param elementCount The number of elements desired.

 @param dataPtr1 The address where the first (or only) region pointer will be
 stored.

 @param sizePtr1 The address where the first (or only) region length will be
 stored.

 @param dataPtr2 The address where the second region pointer will be stored if
 the first region is too small to satisfy elementCount.

 @param sizePtr2 The address where the second region length will be stored if
 the first region is too small to satisfy elementCount.

 @return The number of elements available for reading.
*/
long PaUtil_GetRingBufferReadRegions( PaUtilRingBuffer *rbuf, long elementCount,
                                      void **dataPtr1, long *sizePtr1,
                                      void **dataPtr2, long *sizePtr2 );

/** Advance the read index to the next location to be read.

 @param rbuf The ring buffer.

 @param elementCount The number of elements to advance.

 @return The new position.
*/
long PaUtil_AdvanceRingBufferReadIndex( PaUtilRingBuffer *rbuf, long elementCount );


// MLTEST - define a fixed vector of elements, which may wrap around the end of the buffer.
// the vector will be valid as long as the parent ringbuffer does not wrap and overwrite 
// the available elements. 
//
// TODO this whole thing can be improved with a template element argument

#include <iterator>

template< class T >
class RingBufferElementsVector
{
public:
	long mReadIndex, mNumAvailable, mSmallMask, mElementSizeBytes;
	char *mBuffer;
	
	RingBufferElementsVector< T >(PaUtilRingBuffer *rbuf)
	{
		mReadIndex = rbuf->readIndex;
		mSmallMask = rbuf->smallMask;		
		mNumAvailable = PaUtil_GetRingBufferReadAvailable(rbuf);		
		mElementSizeBytes = rbuf->elementSizeBytes; // must equal sizeof(< T >)
		mBuffer = rbuf->buffer;
	}
	
	inline int size() { return mNumAvailable; }
	
	void* getElementPtr(int n) const
	{
		long elementIndex = (mReadIndex + n) & mSmallMask;
		return mBuffer + elementIndex*mElementSizeBytes;
	}
	
	const T& operator[](int n) const
	{
		void* pVoid = getElementPtr(n);
		T* pElem = static_cast<T*>(pVoid);
		return *pElem;
	}
	
	friend class iterator;
	class iterator : public std::iterator<std::random_access_iterator_tag, T>
	{
		int mIndex;
		RingBufferElementsVector< T >* mpVector;
		
	public:
		iterator(RingBufferElementsVector< T >* p) : mIndex(0), mpVector(p){}
		iterator(RingBufferElementsVector< T >* p, int i) : mIndex(i), mpVector(p){}
		
		bool operator==(const iterator& b) const 
		{ 
			return (mIndex == b.mIndex);
		}
		
		bool operator!=(const iterator& b) const 
		{ 
			return (mIndex != b.mIndex);
		}
		
		T& operator*() const 
		{ 
			void* pVoid = mpVector->getElementPtr(mIndex);
			T* pElem = static_cast<T*>(pVoid);
			return *pElem;
		}
		
		// prefix increment
		iterator& operator++()
		{			
			mIndex++;
			return *this;
		}
		
		// postfix increment
		iterator operator++(int dummy)
		{			
			iterator ret = *this;
			mIndex++;
			return ret;
		}
		
		// prefix decrement
		iterator& operator--()
		{			
			mIndex--;
			return *this;
		}
		
		// postfix decrement
		iterator operator--(int dummy)
		{			
			iterator ret = *this;
			mIndex--;
			return ret;
		}
		
		iterator& operator+=(int n)
		{			
			mIndex += n;
			return *this;
		}
		
		iterator operator+(int n)
		{
			iterator ret = *this;
			ret += n;
			return ret;
		}
		
		iterator& operator-=(int n)
		{			
			mIndex -= n;
			return *this;
		}
		
		iterator operator-(int n)
		{
			iterator ret = *this;
			ret -= n;
			return ret;
		}
		
		ptrdiff_t operator-(const iterator b) { return mIndex - b.mIndex;}
		bool operator<(const iterator b) { return mIndex < b.mIndex; }
		bool operator>(const iterator b) { return mIndex > b.mIndex; }
		bool operator<=(const iterator b) { return mIndex <= b.mIndex; }
		bool operator>=(const iterator b) { return mIndex >= b.mIndex; }
	};	
	
	iterator begin() 
	{
		return iterator(this);
	}
	
	iterator end() 
	{
		return iterator(this, mNumAvailable);
	}
};


#endif /* PA_RINGBUFFER_H */
