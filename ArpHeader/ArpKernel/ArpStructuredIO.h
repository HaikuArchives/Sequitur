/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpStructuredIO.h
 *
 * Read from a BDataIO in somewhat meaningful chunks.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * This is obsolete.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * June 2, 2000:
 *	Created.
 *
 */

#ifndef ARPKERNEL_STRUCTUREDIO_H
#define ARPKERNEL_STRUCTUREDIO_H

#ifndef _DATAIO_H
#include <be/support/DataIO.h>
#endif

typedef enum {
	ARP_LENDIAN_FILE,
	ARP_BENDIAN_FILE,
	ARP_ALWAYS_SWAP_FILE,
	ARP_NEVER_SWAP_FILE
} arp_file_endian;

class ArpStructuredIO {
public:

	ArpStructuredIO(BDataIO* io, bool own=false, size_t bufsize=1024);
	virtual ~ArpStructuredIO();
	
	// ---- Reading ----
	
	void SetReadEndian(arp_file_endian endian);
	
	void StartReadChunk(ssize_t size);
	ssize_t RemainingReadChunk() const;
	ssize_t FinishReadChunk();
	
	ssize_t Read(void *buffer, size_t size);
	
	status_t ReadInt32(int32* out);
	status_t ReadInt16(int16* out);
	status_t ReadInt8(int8* out);
	status_t ReadMidiNumber(uint64* out);
	
	// Returns B_MISMATCHED_VALUES if they don't match, B_OK if they
	// match, and some other error if there was in IO error.
	status_t ReadMatch(const void* data, size_t size);
	
	// Read and ignore 'amount' bytes.
	status_t ReadPad(size_t amount);
	
	// ---- Writing ----
	
	void SetWriteEndian(arp_file_endian endian);
	
	void StartWriteChunk(ssize_t size);
	ssize_t RemainingWriteChunk() const;
	ssize_t FinishWriteChunk();
	
	ssize_t Write(const void *buffer, size_t size);
	ssize_t Flush();
	
	status_t WriteInt32(int32 val);
	status_t WriteInt16(int16 val);
	status_t WriteInt8(int8 val);
	status_t WriteMidiNumber(uint64 val);
	
	// Write 'amount' bytes of value 'value'.
	status_t WritePad(size_t amount, char value=0);
	
private:
	BDataIO* mIO;
	bool mOwned;
	bool mReadSwap;
	bool mWriteSwap;
	bool _reserved0[2];
	
	size_t mBufSize;
	
	char* mReadBuf;
	int32 mReadPos;
	ssize_t mReadSize;
	ssize_t mReadChunk;
	
	char* mWriteBuf;
	int32 mWritePos;
	ssize_t mWriteChunk;
};

#endif
