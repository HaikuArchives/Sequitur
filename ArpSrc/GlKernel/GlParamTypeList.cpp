#include <ArpCore/StlVector.h>
#include "GlPublic/GlParam.h"
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlParamTypeList.h"

class _ParamTypeListData
{
public:
	vector<GlParamType*>		mParams;
	
	_ParamTypeListData()		{ }

	_ParamTypeListData(const _ParamTypeListData& o)
	{
		for (uint32 k = 0; k < o.mParams.size(); k++) {
			mParams.push_back(o.mParams[k]);
		}
	}

	_ParamTypeListData*		Clone() const
	{
		return new _ParamTypeListData(*this);
	}

	bool					ScrubOne(const GlParamType* pt)
	{
		for (uint32 k = 0; k < mParams.size(); k++) {
			if (mParams[k] == pt) {
				mParams.erase(mParams.begin() + k);
				return true;
			}
		}
		return false;		
	}
};

/***************************************************************************
  * GL-PARAM-TYPE-LIST
 ****************************************************************************/
GlParamTypeList::GlParamTypeList()
		: mData(0)
{
	mData = new _ParamTypeListData();
}

GlParamTypeList::~GlParamTypeList()
{
	DeleteAll();
	delete mData;
}

uint32 GlParamTypeList::Size() const
{
	if (!mData) return 0;
	return uint32(mData->mParams.size());
}

const GlParamType* GlParamTypeList::At(uint32 index) const
{
	if (!mData) return 0;
	if (index >= mData->mParams.size()) return 0;
	return mData->mParams[index];
}

const GlParamType* GlParamTypeList::Find(int32 key) const
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->mParams.size(); k++) {
		if (mData->mParams[k] && mData->mParams[k]->Key() == key)
			return mData->mParams[k];
	}
	return 0;
}

status_t GlParamTypeList::Add(GlParamType* param)
{
	if (!param) return B_ERROR;
	if (!mData) return B_NO_MEMORY;
	ArpASSERT(Find(param->Key()) == 0);
	mData->mParams.push_back(param);
	return B_OK;
}

void GlParamTypeList::DeleteAll()
{
	if (!mData) return;
	for (uint32 k = 0; k < mData->mParams.size(); k++)
		delete mData->mParams[k];
	mData->mParams.resize(0);
}

void GlParamTypeList::Print() const
{
	printf("GlParamTypeList\n");
	if (!mData) return;
	for (uint32 k = 0; k < mData->mParams.size(); k++) {
		printf("%ld: ", k);
		mData->mParams[k]->Print();
	}
}
