/*
	
	ArpStructuredIO.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#include "ArpKernel/ArpStructuredIO.h"

#ifndef APRKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#include <ByteOrder.h>

#include <cstring>
#include <cstdlib>

ArpMOD();

ArpStructuredIO::ArpStructuredIO(BDataIO* io, bool own, size_t bufsize)
	: mIO(io), mOwned(own), mReadSwap(false), mWriteSwap(false),
	  mBufSize(bufsize),
	  mReadBuf(new char[mBufSize]), mReadPos(0),
	  mReadSize(B_NO_INIT), mReadChunk(B_NO_INIT),
	  mWriteBuf(new char[mBufSize]), mWritePos(0),
	  mWriteChunk(B_NO_INIT)
{
}

ArpStructuredIO::~ArpStructuredIO()
{
	Flush();
	
	delete[] mWriteBuf;
	delete[] mReadBuf;
	
	if( mOwned ) delete mIO;
}

// --------------------------------------------------------------------

void ArpStructuredIO::SetReadEndian(arp_file_endian endian)
{
	switch(endian) {
#if B_HOST_IS_LENDIAN
		case ARP_LENDIAN_FILE:		mReadSwap = false;		break;
		case ARP_BENDIAN_FILE:		mReadSwap = true;		break;
#else
		case ARP_LENDIAN_FILE:		mReadSwap = true;		break;
		case ARP_BENDIAN_FILE:		mReadSwap = false;		break;
#endif
		case ARP_ALWAYS_SWAP_FILE:	mReadSwap = true;		break;
		case ARP_NEVER_SWAP_FILE:	mReadSwap = false;		break;
	}
}

void ArpStructuredIO::StartReadChunk(ssize_t size)
{
	mReadChunk = size;
}

ssize_t ArpStructuredIO::RemainingReadChunk() const
{
	return mReadChunk;
}

ssize_t ArpStructuredIO::FinishReadChunk()
{
	if (mReadChunk < 0) return mReadChunk;
	if (mReadChunk > 0) {
		status_t err = ReadPad(mReadChunk);
		if (err < B_OK) return err;
	}
	ssize_t remain = mReadChunk;
	mReadChunk = B_NO_INIT;
	return remain;
}
	
ssize_t ArpStructuredIO::Read(void* buffer, size_t size)
{
	int totSize=0;
	char* bufPos = (char*)buffer;
	
	// Take read size out of remaining chunk; if the read extends
	// past this chunk, truncate it and mark the chunk as an error.
	if (mReadChunk >= (ssize_t)size) mReadChunk -= size;
	else if (mReadChunk >= 0) {
		size = mReadChunk;
		mReadChunk = B_MISMATCHED_VALUES;
	}
	
	// First read anything we have sitting in our buffer.
	if( mReadPos < mReadSize ) {
		const size_t readSize = ( (mReadSize-mReadPos) < (ssize_t)size )
								? (mReadSize-mReadPos) : size;
		memcpy(bufPos, mReadBuf + mReadPos, readSize);
		totSize += readSize;
		size -= readSize;
		bufPos += readSize;
		mReadPos += readSize;
	}
	
	// Now we know buffer is empty; read any remaining data either directly
	// or through the buffer, depending on how much is needed.
	if( size > (mBufSize/2) ) {
		ssize_t res = mIO->Read(bufPos, size);
		if( res < 0 ) return res;
		return totSize + res;
	} else if( size > 0 ) {
		ArpASSERT(mReadPos >= mReadSize);
		mReadSize = mIO->Read(mReadBuf, mBufSize);
		mReadPos = 0;
		if( mReadSize < 0 ) return mReadSize;
		const size_t readSize = ( mReadSize < (ssize_t)size ) ? mReadSize : size;
		memcpy(bufPos, mReadBuf + mReadPos, readSize);
		totSize += readSize;
		size -= readSize;
		bufPos += readSize;
		mReadPos += readSize;
	}
	
	return totSize;
}

status_t ArpStructuredIO::ReadInt32(int32* out)
{
	ssize_t res = Read(out, sizeof(int32));
	if (res < B_OK) return res;
	if (res != sizeof(int32)) return B_BAD_VALUE;
	if (mReadSwap) *out = __swap_int32(*out);
	return B_OK;
}

status_t ArpStructuredIO::ReadInt16(int16* out)
{
	ssize_t res = Read(out, sizeof(int16));
	if (res < B_OK) return res;
	if (res != sizeof(int16)) return B_BAD_VALUE;
	if (mReadSwap) *out = __swap_int16(*out);
	return B_OK;
}

status_t ArpStructuredIO::ReadInt8(int8* out)
{
	ssize_t res = Read(out, sizeof(int8));
	if (res < B_OK) return res;
	if (res != sizeof(int8)) return B_BAD_VALUE;
	return B_OK;
}

status_t ArpStructuredIO::ReadMidiNumber(uint64* out)
{
	status_t res;
	uint8 v;

	//printf("Reading MIDI number: 0x");
	
	*out = 0;
	do {
		if ((res=ReadInt8((int8*)&v)) != B_OK) return res;
		//printf("%02x", v);
		*out = ((*out) << 7) + (v&0x7f);
	} while ((v&0x80) != 0);
	
	//printf(" -> 0x%lx\n", *out);
	
	return B_OK;
}

status_t ArpStructuredIO::ReadMatch(const void* data, size_t size)
{
	char buffer[256];
	
	while (size > 0) {
		size_t readSize = size < sizeof(buffer) ? size : sizeof(buffer);
		ssize_t res = Read(buffer, readSize);
		if (res < B_OK) return res != B_MISMATCHED_VALUES ? res : B_ERROR;
		if (res != (ssize_t)readSize) return B_MISMATCHED_VALUES;
		if (memcmp(data, buffer, readSize) != 0) return B_MISMATCHED_VALUES;
		data = ((const char*)data) + readSize;
		size -= readSize;
	}
	
	return B_OK;
}

status_t ArpStructuredIO::ReadPad(size_t amount)
{
	char buffer[1024];
	
	while (amount > 0) {
		size_t readSize = amount < sizeof(buffer) ? amount : sizeof(buffer);
		ssize_t res = Read(buffer, readSize);
		if (res < B_OK) return res != B_MISMATCHED_VALUES ? res : B_ERROR;
		if (res != (ssize_t)readSize) return B_MISMATCHED_VALUES;
		amount -= readSize;
	}
	
	return B_OK;
}

// --------------------------------------------------------------------

void ArpStructuredIO::SetWriteEndian(arp_file_endian endian)
{
	switch(endian) {
#if B_HOST_IS_LENDIAN
		case ARP_LENDIAN_FILE:		mWriteSwap = false;		break;
		case ARP_BENDIAN_FILE:		mWriteSwap = true;		break;
#else
		case ARP_LENDIAN_FILE:		mWriteSwap = true;		break;
		case ARP_BENDIAN_FILE:		mWriteSwap = false;		break;
#endif
		case ARP_ALWAYS_SWAP_FILE:	mWriteSwap = true;		break;
		case ARP_NEVER_SWAP_FILE:	mWriteSwap = false;		break;
	}
}

void ArpStructuredIO::StartWriteChunk(ssize_t size)
{
	mWriteChunk = size;
}

ssize_t ArpStructuredIO::RemainingWriteChunk() const
{
	return mWriteChunk;
}

ssize_t ArpStructuredIO::FinishWriteChunk()
{
	if (mWriteChunk < 0) return mWriteChunk;
	if (mWriteChunk > 0) {
		status_t err = WritePad(mWriteChunk);
		if (err < B_OK) return err;
	}
	ssize_t remain = mWriteChunk;
	mWriteChunk = B_NO_INIT;
	return remain;
}
	
ssize_t ArpStructuredIO::Write(const void* buffer, size_t size)
{
	const size_t origSize = size;
	const char* bufPos = (const char*)buffer;
	
	// Take write size out of remaining chunk; if the write extends
	// past this chunk, truncate it and mark the chunk as an error.
	if (mWriteChunk >= (ssize_t)size) mWriteChunk -= size;
	else if (mWriteChunk >= 0) {
		size = mWriteChunk;
		mWriteChunk = B_MISMATCHED_VALUES;
	}
	
	if (size > (mBufSize/2)) {
		ssize_t res = Flush();
		if (res < B_OK) return res;
		return mIO->Write(buffer, size);
	}
	
	const size_t writeSize = ( ((ssize_t)mBufSize-mWritePos) < (ssize_t)size )
							   ? (mBufSize-mWritePos) : size;
	if (writeSize > 0) {
		memcpy(&mWriteBuf[mWritePos], bufPos, writeSize);
		size -= writeSize;
		bufPos += writeSize;
		mWritePos += writeSize;
	}
	
	if (mWritePos >= (ssize_t)mBufSize) {
		ssize_t res = Flush();
		if (res < B_OK) return res;
	}
	
	if (size > 0) {
		ArpASSERT(mWritePos == 0);
		memcpy(&mWriteBuf[mWritePos], bufPos, size);
		mWritePos += size;
	}
	
	return origSize;
}

ssize_t ArpStructuredIO::Flush()
{
	if (mWritePos > 0) {
		ssize_t res = mIO->Write(mWriteBuf, mWritePos);
		mWritePos = 0;
		return res;
	}
	
	return B_OK;
}

status_t ArpStructuredIO::WriteInt32(int32 value)
{
	if (mWriteSwap) value = __swap_int32(value);
	ssize_t res = Write(&value, sizeof(int32));
	if (res < B_OK) return res;
	if (res != sizeof(int32)) return B_BAD_VALUE;
	return B_OK;
}

status_t ArpStructuredIO::WriteInt16(int16 value)
{
	if (mWriteSwap) value = __swap_int16(value);
	ssize_t res = Write(&value, sizeof(int16));
	if (res < B_OK) return res;
	if (res != sizeof(int16)) return B_BAD_VALUE;
	return B_OK;
}

status_t ArpStructuredIO::WriteInt8(int8 value)
{
	ssize_t res = Write(&value, sizeof(int8));
	if (res < B_OK) return res;
	if (res != sizeof(int8)) return B_BAD_VALUE;
	return B_OK;
}

status_t ArpStructuredIO::WriteMidiNumber(uint64 value)
{
	uint8 buffer[32];
	int32 i = sizeof(buffer)-1;
	buffer[i] = value&0x7f;
	while ((value >>= 7) != 0) {
		i--;
		buffer[i] = (value&0x7f) | 0x80;
	}
	
	ssize_t res = Write(buffer+i, sizeof(buffer)-i);
	return res >= B_OK ? B_OK : res;
}

status_t ArpStructuredIO::WritePad(size_t amount, char value)
{
	char buffer[1024];
	
	memset(buffer, value, amount < sizeof(buffer) ? amount : sizeof(buffer));
	
	while (amount > 0) {
		size_t writeSize = amount < sizeof(buffer) ? amount : sizeof(buffer);
		ssize_t res = Write(buffer, writeSize);
		if (res < B_OK) return res != B_MISMATCHED_VALUES ? res : B_ERROR;
		if (res != (ssize_t)writeSize) return B_MISMATCHED_VALUES;
		amount -= writeSize;
	}
	
	return B_OK;
}
