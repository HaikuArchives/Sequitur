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
 * SBFWriter.h
 *
 * Classes for writing ARP SBF files.
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

#ifndef SBF_SBFWRITER_H
#define SBF_SBFWRITER_H

#ifndef SBF_SBFSTRUCTS_H
#include <SBF/SBFStructs.h>
#endif

#include <map>

namespace ArpSBF {

class FileContext;

class ObjectBase {
public:
	ObjectBase()
	
	SetContext(FileContext* context);
	FileContext* Context() const;
	
	uint32 ObjectID();
	
private:
	FileContext*	mContext;
	uint32			mID;
};

class SBFContext {
public:
	SBFContext(BDataIO& io);
	SBFContext(BPositionIO& io)

	uint32 ObjectToID(void* object)
	void* IDToObject(uint32 id) const;
	
private:
	BDataIO&				mIO;
	BPositionIO*			mPIO;
	std::map<uint32, void*>	mIDToObject;
	std::map<void*, uint32>	mObjectToID;
};

class ChunkWriter {
public:
	// Constructor for writing initial chunk.
	ChunkWriter(SBFContext& context);
	
	// Constructor for starting a new chunk in an existing file.
	SBFChunkWriter(SBFChunkWriter& parent);

	// Chunk header initialization.
	status_t SetFullHeader(	uint32 type, const char* name, uint16 flags,
							uint32 object=0);
	
private:
	SBFContext&		mContext;
	SBFChunkWriter*	mParent;
};

#endif
