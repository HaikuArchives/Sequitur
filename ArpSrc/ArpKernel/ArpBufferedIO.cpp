/*
	
	ArpBufferedIO.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPKERNEL_ARPBUFFEREDIO_H
#include "ArpKernel/ArpBufferedIO.h"
#endif

#ifndef APRKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#include <string.h>
#include <stdlib.h>

ArpMOD();

ArpBufferedIO::ArpBufferedIO(BDataIO* io, bool own, size_t bufsize)
	: mIO(io), mOwned(own), mBufSize(bufsize),
	  mReadBuf(new char[mBufSize]), mReadPos(0), mReadSize(B_NO_INIT),
	  mWriteBuf(new char[mBufSize]), mWritePos(0)
{
}

ArpBufferedIO::~ArpBufferedIO()
{
	Flush();
	
	delete[] mWriteBuf;
	delete[] mReadBuf;
	
	if( mOwned ) delete mIO;
}

ssize_t ArpBufferedIO::Read(void* buffer, size_t size)
{
	int totSize=0;
	char* bufPos = (char*)buffer;
	
	// First read anything we have sitting in our buffer.
	if( mReadPos < mReadSize ) {
		const int readSize = ( (mReadSize-mReadPos) < (int)size )
								? (mReadSize-mReadPos) : size;
		memcpy(bufPos, &mReadBuf[mReadPos], readSize);
		totSize += readSize;
		size -= readSize;
		bufPos += readSize;
		mReadPos += readSize;
	}
	
	// Now we know buffer is empty; read any remaining data either directly
	// or through the buffer, depending on how much is needed.
	if( size > (mBufSize/4) ) {
		ssize_t res = mIO->Read(bufPos, size);
		if( res < 0 ) return res;
		return totSize + res;
	} else if( size > 0 ) {
		ArpASSERT(mReadPos >= mReadSize);
		mReadSize = mIO->Read(mReadBuf, mBufSize);
		mReadPos = 0;
		if( mReadSize < 0 ) return mReadSize;
		const int readSize = ( mReadSize < (int)size ) ? mReadSize : size;
		memcpy(bufPos, &mReadBuf[mReadPos], readSize);
		totSize += readSize;
		size -= readSize;
		bufPos += readSize;
		mReadPos += readSize;
	}
	
	return totSize;
}

ssize_t ArpBufferedIO::Write(const void* buffer, size_t size)
{
	int totSize=0;
	const char* bufPos = (const char*)buffer;
	
	// Will this all fit into the write buffer?
	if( (mBufSize-mWritePos) < size ) {
		memcpy(&mWriteBuf[mWritePos], bufPos, size);
		mWritePos += size;
		return size;
	}
	
	// Should we fill up the existing buffer before writing?
	if( (size - (mBufSize-mWritePos)) > (mBufSize/4) ) {
		const int remSize = mBufSize-mWritePos;
		memcpy(&mWriteBuf[mWritePos], bufPos, remSize);
		totSize += remSize;
		size -= remSize;
		bufPos += remSize;
		mReadPos += remSize;
	}
	
	// Write out existing buffer.
	Flush();
	
	// Now just figure out what to do with the remaining data -- either write
	// it directly, or copy into the buffer, depending on how much there is.
	if( size > (mBufSize/4) ) {
		ssize_t res = mIO->Write(bufPos, size);
		if( res < 0 ) return res;
		return totSize + res;
	} else if( size > 0 ) {
		ArpASSERT( (mBufSize-mWritePos) < size );
		memcpy(&mWriteBuf[mWritePos], bufPos, size);
		totSize += size;
		size = 0;
		bufPos += size;
		mWritePos += size;
	}
	
	return totSize;
}

status_t ArpBufferedIO::Flush()
{
	ssize_t ret = B_NO_ERROR;
	
	if( mWritePos > 0 ) {
		// keep on writing until the buffer is empty, nothing is written,
		// or we get an error.
		int32 curPos=0, lastPos;
		do {
			lastPos = curPos;
			ret = mIO->Write(&mWriteBuf[curPos], mWritePos-curPos);
			if( ret > 0 ) curPos += ret;
		} while( curPos != lastPos );
		if( curPos > 0 && curPos < mWritePos ) {
			memcpy(mWriteBuf,&mWriteBuf[curPos], mWritePos-curPos);
		}
		if( curPos > 0 ) mWritePos -= curPos;
	}
	
	return ret;
}
