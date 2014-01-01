#include <ArpCore/StlVector.h>
#include "ArpMath/ArpDefs.h"
#include "GlPublic/GlImage.h"
#include "GlPublic/GlMaskMap.h"

class _MaskMapEntry
{
public:
	gl_id		id;
	int32		w, h;

	_MaskMapEntry()	: id(0), w(0), h(0)							{ }

	_MaskMapEntry(gl_id inId, uint8* inData, int32 inW, int32 inH)
			: id(inId), w(inW), h(inH)
	{
		if (inData) mData_u8.push_back(inData);
	}

	_MaskMapEntry(gl_id inId, float* inData, int32 inW, int32 inH)
			: id(inId), w(inW), h(inH)
	{
		if (inData) mData_f.push_back(inData);
	}

	~_MaskMapEntry()
	{
		uint32		k;
		for (k = 0; k < mData_u8.size(); k++) delete mData_u8[k];
		for (k = 0; k < mData_f.size(); k++) delete mData_f[k];
	}

	uint32 FrameCount() const
	{
		ArpASSERT(mData_u8.size() < 1 || mData_f.size() < 1);
		return uint32(ARP_MAX(mData_u8.size(), mData_f.size()));
	}
	
	uint8* GetMask(int32 frame)
	{
		if (frame < 0 || frame >= int32(mData_u8.size())) return 0;
		return mData_u8[frame];
	}

	float* GetMaskF(int32 frame)
	{
		if (frame < 0 || frame >= int32(mData_f.size())) return 0;
		return mData_f[frame];
	}

	void AddMask(uint8* data)
	{
		ArpASSERT(mData_f.size() == 0);
		mData_u8.push_back(data);
	}

	void AddMaskF(float* data)
	{
		ArpASSERT(mData_u8.size() == 0);
		mData_f.push_back(data);
	}

private:
	vector<uint8*>	mData_u8;
	vector<float*>	mData_f;
};

class _MaskMapData
{
public:
	_MaskMapData()		{ }
	~_MaskMapData()
	{
		uint32			k;
		for (k = 0; k < mEntries.size(); k++) delete mEntries[k];
	}

	uint32 FrameCount(gl_id id) const
	{
		for (uint32 k = 0; k < mEntries.size(); k++) {
			if (mEntries[k] && mEntries[k]->id == id) {
				return mEntries[k]->FrameCount();
			}
		}
		return 0;
	}

	/* Answer the mask at the frame. Frame 0, is a special case,
	 * it creates a new mask if it has to.
	 */
	uint8* GetMask(gl_id id, int32 w, int32 h, int32 frame)
	{
		for (uint32 k = 0; k < mEntries.size(); k++) {
			if (mEntries[k] && mEntries[k]->id == id) {
				if (mEntries[k]->w == w && mEntries[k]->h == h) return mEntries[k]->GetMask(frame);
				return 0;
			}
		}
		if (frame == 0) return NewMask(id, w, h);
		return 0;
	}

	/* Answer the mask at the frame. Frame 0, is a special case,
	 * it creates a new mask if it has to.
	 */
	float* GetMaskF(gl_id id, int32 w, int32 h, int32 frame)
	{
		for (uint32 k = 0; k < mEntries.size(); k++) {
			if (mEntries[k] && mEntries[k]->id == id) {
				if (mEntries[k]->w == w && mEntries[k]->h == h) return mEntries[k]->GetMaskF(frame);
				return 0;
			}
		}
		if (frame == 0) return NewMaskF(id, w, h);
		return 0;
	}

	uint8* NewMask(gl_id id, int32 w, int32 h)
	{
		uint8*			data = new uint8[w * h];
		if (!data) return 0;

		for (uint32 k = 0; k < mEntries.size(); k++) {
			if (mEntries[k] && mEntries[k]->id == id) {
				ArpASSERT(mEntries[k]->w == w && mEntries[k]->h == h);
				if (mEntries[k]->w != w || mEntries[k]->h != h) {
					delete data;
					return 0;
				}
				mEntries[k]->AddMask(data);
				return data;
			}
		}

		_MaskMapEntry*	entry = new _MaskMapEntry(id, data, w, h);
		if (!entry) {
			delete data;
			return 0;
		}
		mEntries.push_back(entry);
		return data;
	}

	float* NewMaskF(gl_id id, int32 w, int32 h)
	{
		float*			data = new float[w * h];
		if (!data) return 0;

		for (uint32 k = 0; k < mEntries.size(); k++) {
			if (mEntries[k] && mEntries[k]->id == id) {
				ArpASSERT(mEntries[k]->w == w && mEntries[k]->h == h);
				if (mEntries[k]->w != w || mEntries[k]->h != h) {
					delete data;
					return 0;
				}
				mEntries[k]->AddMaskF(data);
				return data;
			}
		}

		_MaskMapEntry*	entry = new _MaskMapEntry(id, data, w, h);
		if (!entry) {
			delete data;
			return 0;
		}
		mEntries.push_back(entry);
		return data;
	}

private:
	vector<_MaskMapEntry*>	mEntries;
};

/***************************************************************************
 * GL-MASK-MAP
 ****************************************************************************/
GlMaskMap::GlMaskMap()
		: mData(0)
{
	mData = new _MaskMapData();
}

GlMaskMap::~GlMaskMap()
{
	delete mData;
}

uint32 GlMaskMap::FrameCount(gl_id id) const
{
	if (!mData) return 0;
	return mData->FrameCount(id);
}

uint8* GlMaskMap::GetMask(const GlImage* image, int32 frame)
{
	if (!mData || !image) return 0;
	return GetMask(image->Id(), image->Width(), image->Height(), frame);
}

uint8* GlMaskMap::GetMask(gl_id id, int32 w, int32 h, int32 frame)
{
	if (!mData) return 0;
	if (frame < 0) return mData->NewMask(id, w, h);
	return mData->GetMask(id, w, h, frame);
}

float* GlMaskMap::GetMaskF(const GlImage* image, int32 frame)
{
	if (!mData || !image) return 0;
	return GetMaskF(image->Id(), image->Width(), image->Height(), frame);
}

float* GlMaskMap::GetMaskF(gl_id id, int32 w, int32 h, int32 frame)
{
	if (!mData) return 0;
	if (frame < 0) return mData->NewMaskF(id, w, h);
	return mData->GetMaskF(id, w, h, frame);
}
	
void GlMaskMap::MakeEmpty()
{
	delete mData;
	mData = new _MaskMapData();
}
