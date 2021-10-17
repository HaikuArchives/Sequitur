#include <cstdio>
#include <ArpCore/StlVector.h>
#include "GlPublic/GlParam.h"
#include "GlPublic/GlParamList.h"
#include "GlPublic/GlParamType.h"
#include "GlPublic/GlParamTypeList.h"
#include "GlPublic/GlStrainedParamList.h"

static const char*		PARAM_TYPE_KEY_STR			= "ky";
static const char*		_NODE_PARAMS_STR			= "node_params";

/*************************************************************************
 * _GL-PARAM-LIST-DATA
 *************************************************************************/
class _GlParamListData
{
public:
	std::vector<GlParam*>		params;

	_GlParamListData();
	_GlParamListData(const _GlParamListData& o, gl_node_id nid);
	~_GlParamListData();

	const GlParam*			Find(int32 key, int32 i) const
	{
		int32		match = 0;
		for (uint32 k = 0; k < params.size(); k++) {
			if (params[k] && params[k]->ParamType() && key == params[k]->ParamType()->Key()) {
				if (match == i) return params[k];
				match++;
			}
		}
		return 0;
	}
};

/*************************************************************************
 * GL-PARAM-LIST
 *************************************************************************/
GlParamList::GlParamList(	gl_id owner, const GlParamTypeList* paramTypes,
							const char* name)
		: mParamsChanged(false), mData(new _GlParamListData()),
		  mParamTypes(paramTypes), mOwner(owner), mName(name)
{
	if (!mName) mName = _NODE_PARAMS_STR;
	ArpASSERT(mName);
}

GlParamList::GlParamList(const GlParamList& o, gl_id owner)
		: mParamsChanged(false), mData(0), mParamTypes(o.mParamTypes),
		  mOwner(owner), mName(o.mName)
{
	if (o.mData) mData = new _GlParamListData(*(o.mData), mOwner);
	else mData = new _GlParamListData();
	ArpASSERT(mName);
}

GlParamList::~GlParamList()
{
	delete mData;
}

uint32 GlParamList::Size() const
{
	if (!mData) return 0;
	return uint32(mData->params.size());
}

const GlParam* GlParamList::At(uint32 index) const
{
	if (!mData) return 0;
	if (index >= mData->params.size()) return 0;
	return mData->params[index];
}

GlParam* GlParamList::At(uint32 index)
{
	if (!mData) return 0;
	if (index >= mData->params.size()) return 0;
	return mData->params[index];
}

const GlParam* GlParamList::Find(int32 ptKey, int32 i) const
{
	if (!mData) return 0;
	int32		match = 0;
	for (uint32 k = 0; k < mData->params.size(); k++) {
		if (mData->params[k] && mData->params[k]->ParamType()
				&& mData->params[k]->ParamType()->Key() == ptKey) {
			if (match == i) return mData->params[k];
			match++;
		}
	}
	return 0;
}

GlParam* GlParamList::Find(int32 ptKey, int32 i)
{
	if (!mData) return 0;
	int32		match = 0;
	for (uint32 k = 0; k < mData->params.size(); k++) {
		if (mData->params[k] && mData->params[k]->ParamType()
				&& mData->params[k]->ParamType()->Key() == ptKey) {
			if (match == i) return mData->params[k];
			match++;
		}
	}
	return 0;
}

#if 0
	const GlParam*		FindParam(const GlParamType* pt, int32 i = 0) const;
	GlParam*			FindParam(const GlParamType* pt, int32 i = 0);

const GlParam* GlParamList::FindParam(const GlParamType* pt, int32 i) const
{
	if (!mData) return 0;
	int32		match = 0;
	for (uint32 k = 0; k < mData->params.size(); k++) {
		if (mData->params[k] && mData->params[k]->ParamType() == pt) {
			if (match == i) return mData->params[k];
			match++;
		}
	}
	return 0;
}

GlParam* GlParamList::FindParam(const GlParamType* pt, int32 i)
{
	if (!mData) return 0;
	int32		match = 0;
	for (uint32 k = 0; k < mData->params.size(); k++) {
		if (mData->params[k] && mData->params[k]->ParamType() == pt) {
			if (match == i) return mData->params[k];
			match++;
		}
	}
	return 0;
}
#endif

status_t GlParamList::ReadFrom(const BMessage& config)
{
	ArpASSERT(mName);
	ArpVALIDATE(mOwner && mParamTypes, return B_ERROR);
	delete mData;
	mData = new _GlParamListData();
	if (!mData) return B_NO_MEMORY;
	BMessage						allmsg;

	if (config.FindMessage(mName, &allmsg) == B_OK) {
		mParamsChanged = true;
		BMessage					msg;
		for (int32 pi = 0; allmsg.FindMessage("p", pi, &msg) == B_OK; pi++) {
			int32					key;
			if (msg.FindInt32(PARAM_TYPE_KEY_STR, &key) == B_OK) {
				const GlParamType*	paramType = mParamTypes->Find(key);
				if (paramType) {
					GlParam*		param = paramType->NewParam(mOwner, &msg);
					if (param) mData->params.push_back(param);
				}
			}
			msg.MakeEmpty();
		}
	}
	return B_OK;
}

status_t GlParamList::WriteTo(BMessage& config) const
{
	ArpVALIDATE(mData && mOwner, return B_ERROR);
	ArpASSERT(mName);
	status_t		err;
	if (mParamsChanged) {
		BMessage	allmsg;
		for (uint32 k = 0; k < mData->params.size(); k++) {
			/* Only store params based on this node -- anything
			 * else is a result of the somewhat complex matrix node,
			 * and should be handled there.
			 */
			GlParam*			p = mData->params[k];
			const GlParamType*	pt = p->ParamType();
			if (mOwner == p->Owner() && p->IsValid()) {
				BMessage		msg;
				if ((err = msg.AddInt32(PARAM_TYPE_KEY_STR, pt->Key())) != B_OK) return err;
				if ((err = mData->params[k]->WriteTo(msg)) != B_OK) return err;
				if ((err = allmsg.AddMessage("p", &msg)) != B_OK) return err;
			}
		}
		if ((err = config.AddMessage(mName, &allmsg)) != B_OK) return err;
	}
	return B_OK;
}

status_t GlParamList::AddParam(GlParam* param, bool allowDups)
{
	if (!mData) return B_ERROR;
	ArpASSERT(param->ParamType());
	if (!allowDups) ArpASSERT(Find(param->ParamType()->Key()) == 0);
	mData->params.push_back(param);
	return B_OK;
}

status_t GlParamList::MakeEmpty()
{
	if (!mData) return B_ERROR;
	for (uint32 k = 0; k < mData->params.size(); k++) delete mData->params[k];
	mData->params.resize(0);
	return B_OK;
}

bool GlParamList::Bool(int32 key, bool init, int32 i) const
{
	GlBoolWrap			w(init);
	GetValue(0, key, w, i);
	return w.v;
}

ArpVoxel GlParamList::Color(int32 key, int32 i) const
{
	GlColorWrap			w(255, 255, 255);
	GetValue(0, key, w, i);
	return w.v;
}

BString16 GlParamList::FileName(int32 key, int32 i) const
{
	GlTextWrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

float GlParamList::Float(int32 key, int32 i) const
{
	GlFloatWrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

ArpFont GlParamList::Font(int32 key, int32 i) const
{
	GlFontWrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

int32 GlParamList::Int32(int32 key, int32 i) const
{
	GlInt32Wrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

int32 GlParamList::Menu(int32 key, int32 i) const
{
	GlInt32Wrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

BPoint GlParamList::Point(int32 key, int32 i) const
{
	GlPointWrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

ArpPoint3d GlParamList::Point3d(int32 key, int32 i) const
{
	GlPoint3dWrap		w;
	GetValue(0, key, w, i);
	return w.v;
}

GlRelAbs GlParamList::RelAbs(int32 key, const GlRelAbs* init, int32 i) const
{
	GlRelAbsWrap		w;
	if (init) w.v = *init;
	GetValue(0, key, w, i);
	return w.v;
}

BString16 GlParamList::Text(int32 key, int32 i) const
{
	GlTextWrap			w;
	GetValue(0, key, w, i);
	return w.v;
}

status_t GlParamList::SetValue(int32 key, const GlParamWrap& wrap, int32 i)
{
	if (!mData) return B_ERROR;
	uint32				k;
	int32				match = 0;
	for (k = 0; k < mData->params.size(); k++) {
		GlParam*		p = mData->params[k];
		if (p && p->ParamType() && key == p->ParamType()->Key()) {
			if (match == i) {
				wrap.SetValue(p);
				mParamsChanged = true;
				return B_OK;
			}
			match++;
		}
	}
	if (!mParamTypes) return B_ERROR;
	const GlParamType*	pt = mParamTypes->Find(key);
	if (!pt) return B_ERROR;
	GlParam*			p = pt->NewParam(mOwner);
	if (!p) return B_NO_MEMORY;
	wrap.SetValue(p);
	/* FIX:  Need to clean this thing up, since now I always allow duplicates.
	 */
	AddParam(p, true);
	mParamsChanged = true;
	return B_OK;
}

status_t GlParamList::GetValue(	const gl_process_args* args, int32 ptKey,
								GlParamWrap& outWrap, int32 i) const
{
	if (GetValueNoInit(args, ptKey, outWrap, i) == B_OK) return B_OK;
	/* See if I have a paramType at the key.
	 */
	if (!mParamTypes) return B_ERROR;
	const GlParamType*	pt = mParamTypes->Find(ptKey);
	if (!pt) return B_ERROR;
	return pt->GetInit(outWrap);

	return B_ERROR;
}

status_t GlParamList::GetValueNoInit(	const gl_process_args* args, int32 ptKey,
										GlParamWrap& outWrap, int32 i) const
{
	/* Check to see if the args have a param for the paramType.
	 */
	if (args && args->paramList && mOwner) {
		const GlParam*	p = args->paramList->At(gl_param_key(mOwner, ptKey, i));
		if (p) return outWrap.GetValue(p);
	}
	/* Check to see if I have a param at the key.
	 */
	return GetParamValue(ptKey, outWrap, i);
}

status_t GlParamList::EraseValue(int32 key, int32 i)
{
	if (!mData) return B_ERROR;
	uint32				k;
	int32				match = 0;
	for (k = 0; k < mData->params.size(); k++) {
		GlParam*		p = mData->params[k];
		if (p && p->ParamType() && key == p->ParamType()->Key()) {
			if (match == i) {
				mData->params.erase(mData->params.begin() + k);
				delete p;
				return B_OK;
			}
			match++;
		}
	}
	return B_ERROR;
}

status_t GlParamList::ExtractParamValue(	const gl_process_args* args,
											int32 ptKey, GlParamWrap& wrap,
											int32 i) const
{
	/* First check to see if the args have a param for the paramType.
	 */
	if (args && args->paramList && mOwner) {
		const GlParam*	p = args->paramList->At(gl_param_key(mOwner, ptKey, i));
		if (p) return wrap.GetValue(p);
	}
	/* If not, see if I have a param for the paramType.
	 */
	if (!mData) return B_ERROR;
	int32				match = 0;
	for (uint32 k = 0; k < mData->params.size(); k++) {
		GlParam*		p = mData->params[k];
		if (p && p->ParamType() && ptKey == p->ParamType()->Key()) {
			if (i == match) return wrap.GetValue(p);
			match++;
		}
	}
	return B_ERROR;
}

status_t GlParamList::GetParamValue(int32 ptKey, GlParamWrap& outWrap, int32 i) const
{
	if (!mData) return B_ERROR;
	int32				match = 0;
	for (uint32 k = 0; k < mData->params.size(); k++) {
		GlParam*		p = mData->params[k];
		if (p && p->ParamType() && ptKey == p->ParamType()->Key()) {
			if (match == i) return outWrap.GetValue(p);
			match++;
		}
	}
	return B_ERROR;
}

void GlParamList::Print() const
{
	printf("GlParamList\n");
	if (!mData) {
		printf("<empty>\n");
		return;
	}
	for (uint32 k = 0; k < mData->params.size(); k++) {
		GlParam*		p = mData->params[k];
		if (p) {
			printf("%ld: ", k);
			p->Print();
		}
	}
}

// #pragma mark -

/*************************************************************************
 * _GL-PARAM-LIST-DATA
 *************************************************************************/
_GlParamListData::_GlParamListData()
{
}

_GlParamListData::_GlParamListData(const _GlParamListData& o, gl_node_id nid)
{
	for (uint32 k = 0; k < o.params.size(); k++) {
		GlParam*	p = o.params[k]->Clone(nid);
		if (p) params.push_back(p);
	}
}

_GlParamListData::~_GlParamListData()
{
	for (uint32 k = 0; k < params.size(); k++) delete params[k];
	params.resize(0);
}
