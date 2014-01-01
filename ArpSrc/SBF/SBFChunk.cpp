#ifndef SBF_SBFCHUNK_H
#include <SBF/SBFChunk.h>
#endif

#ifndef SBF_SBFCRC_H
#include <SBF/SBFcrc.h>
#endif

class SBFCRCDataWrapper : public BDataIO
{
public:
	SBFCRCDataWrapper(BDataIO* io, uint32* crc, bool writing)
		: mIO(io), mCRC(crc), mWriting(writing)
		{ }
	
	virtual	ssize_t Read(void *buffer, size_t size) {
		if( mWriting ) return B_ERROR;
		ssize_t ret = mIO->Read(buffer, size);
		*mCRC = SBFUpdateCRC(*mCRC, (uint8*)buffer, size);
		return ret;
	}
	virtual	ssize_t Write(const void *buffer, size_t size) {
		if( !mWriting ) return B_ERROR;
		*mCRC = SBFUpdateCRC(*mCRC, (uint8)buffer, size);
		return mIO->Write(buffer, size);
	}

private:
	BDataIO*	mIO;
	uint32*		mCRC;
	bool		mWriting;
};

class SBFCRCMallocWrapper : public SBFCRCDataWrapper
{
public:
	SBFCRCMallocWrapper(uint32* crc)
		: SBFCRCDataWrapper( (mMalloc=new BMallocIO), crc, true)
		{ }
	~SBFCRCMallocWrapper() {
		delete mMalloc;
	}

private:
	BMallocIO*	mMalloc;
};

SBFBasicChunkW::SBFBasicChunkW(BDataIO* stream, const char* name,
							   uint16 flags, uint32 type = SBFChunkHead)
{
	mDataIO = stream;
	mPositionIO = 0;
	mBuffer = 0;
	Initialize(name, flags, type);
	mBuffer = new SBFCRCMallocWrapper(&mTail.mCRC);
}

SBFBasicChunkW::SBFBasicChunkW(BPositionIO* stream, const char* name,
							   uint16 flags, uint32 type = SBFChunkHead)
{
	mDataIO = stream;
	mPositionIO = stream;
	mBuffer = 0;
	Initialize(name, flags, type);
	mBuffer = new SBFCRCDataWrapper(mDataIO, &mTail.mCRC, true);
}

void SBFBasicChunkW::Initialize(const char* name, uint16 flags, uint32 type)
{
	mHead.mType = type;
	memset(&mHead.mName[0], sizeof(mHead.mName), 0);
	if( name ) strncat(&mHead.mName[0], name, sizeof(mHead.mName);
	mHead.mFlags = flags;
	mHead.mHeadLength = sizeof(mHead);
	mHead.mTailLength = sizeof(mTail);
	mHead.mObject = 0;
	mHead.mLength = 0;
	mHead.mContinuation = 0;
	mTail.mCRC = 0xffffffffL;
}
