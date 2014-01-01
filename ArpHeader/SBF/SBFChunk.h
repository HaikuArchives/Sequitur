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
 * ArpSFFStructs.h
 *
 * Definition of the structures used in ARP's "structured file format".
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * Still in development.
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
 * Feb. 6, 1999:
 *	Created this file.
 *
 */

#ifndef SBF_SBFCHUNK_H
#define SBF_SBFCHUNK_H

#ifndef SBF_SBFSTRUCTS_H
#include <SBF/SBFStructs.h>
#endif

class SBFChunkI {
public:
	virtual uint32 Type() const							= 0;
	
	virtual void GetName(BString* result) const			= 0;
	virtual bool IsName(const char* name) const			= 0;
	
	virtual uint16 Flags() const						= 0;
	
	virtual uint32 Object() const						= 0;
	
	virtual int64 Length() const						= 0;
	
	virtual const BDataIO* Buffer() const				= 0;
	virtual BDataIO* Buffer()							= 0;
};

class SBFChunkReaderI {
public:
	status_t SBFReadChunk(const SBFChunkI* chunk)		= 0;
	status_t SBFReadChunk(ubyte8 flags, ubyte8 type,
						  uint16 length, ubyte8* data)	= 0;
};

class SBFBasicChunkW : public SBFChunkI {
public:
	SBFBasicChunkW(BDataIO* stream, const char* name,
				   uint16 flags, uint32 type = SBFChunkHead::BASIC_TYPE);
	SBFChunk(BPositionIO* stream, const char* name,
			 uint16 flags, uint32 type = SBFChunkHead::BASIC_TYPE);

	virtual uint32 Type() const;
	
	virtual void GetName(BString* return) const;
	virtual bool IsName(const char* name) const;
	
	virtual uint16 Flags() const;
	
	virtual uint32 Object() const;
	
	virtual int64 Length() const;
	
	virtual const BDataIO* Buffer() const;
	virtual BDataIO* Buffer();
	
private:
	void Initialize(const char* name, uint16 flags, uint32 type);
	
	struct SBFChunkHead		mHead;
	struct SBFChunkTail		mTail;
	
	BDataIO*				mDataIO;
	BPositionIO*			mPositionIO;
	
	BDataIO*				mBuffer;
};

#endif
