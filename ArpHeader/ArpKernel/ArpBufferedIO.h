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
 * ArpBufferedIO.h
 *
 * This was going to be a wrapper around raw BDataIO objects, but then
 * R4 showed up and ahhh.....
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
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef ARPKERNEL_BUFFEREDIO_H
#define ARPKERNEL_BUFFEREDIO_H

#ifndef _DATAIO_H
#include <support/DataIO.h>
#endif

class ArpBufferedIO : public BDataIO {
private:
	typedef BDataIO inherited;

public:

	ArpBufferedIO(BDataIO* io, bool own=false, size_t bufsize=1024);
	virtual ~ArpBufferedIO();
	
	virtual	ssize_t Read(void *buffer, size_t size);
	virtual	ssize_t Write(const void *buffer, size_t size);
	
	virtual status_t Flush();
	
private:
	BDataIO* mIO;
	bool mOwned;
	
	size_t mBufSize;
	
	char* mReadBuf;
	int32 mReadPos;
	ssize_t mReadSize;
	
	char* mWriteBuf;
	int32 mWritePos;
};

#endif
