#include <ArpCore/StlVector.h>
#include <GlPublic/GlNodeData.h>
#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlAlgoNbrInput.h>

static std::vector<int32>		gKeys;

/***************************************************************************
 * GL-ALGO-NBR
 ****************************************************************************/
GlAlgoNbr::~GlAlgoNbr()
{
}

GlAlgoNbr* GlAlgoNbr::AsNbr()
{
	return this;
}

const GlAlgoNbr* GlAlgoNbr::AsNbr() const
{
	return this;
}

GlAlgoNbrIn* GlAlgoNbr::AsInput()
{
	return 0;
}

int32 GlAlgoNbr::Key() const
{
	return mKey;
}

GlAlgoNbr::GlAlgoNbr(int32 key, gl_node_id nid, int32 token)
		: inherited(nid, token), mKey(key)
{
}

GlAlgoNbr::GlAlgoNbr(const GlAlgoNbr& o)
		: inherited(o), mKey(o.mKey)
{
}

bool GlAlgoNbr::RegisterKey(int32 key)
{
	for (uint32 k = 0; k < gKeys.size(); k++) {
		if (key == gKeys[k]) return false;
	}
	gKeys.push_back(key);
	return true;
}

// #pragma mark -

/*******************************************************
 * GL-ALGO-NBR-WRAP
 *******************************************************/
GlAlgoNbrWrap::GlAlgoNbrWrap(GlAlgo* a)
		: mCache(0), mSize(0)
{
	if (a) SetAlgo(a);
}

GlAlgoNbrWrap::~GlAlgoNbrWrap()
{
	Free();
}

status_t GlAlgoNbrWrap::InitCheck() const
{
	if (mCache && mSize > 0) return B_OK;
	return B_ERROR;
}

status_t GlAlgoNbrWrap::SetAlgo(GlAlgo* algo)
{
	Free();
	if (!algo) return B_ERROR;
	
	GlAlgo*				a = algo;
	uint32				k, cur, c = 0;
	while (a) {
		c++;
		a = a->mNext;
	}
	ArpASSERT(c > 0);
	mCache = new GlAlgoNbr*[c];
	if (!mCache) return B_NO_MEMORY;
	
	a = algo;
	cur = 0;
	for (k = 0; k < c; k++) {
		mCache[cur] = 0;
		if (a) {
			mCache[cur] = a->AsNbr();
			a = a->mNext;
			if (mCache[cur]) cur++;
		}
	}
	mSize = cur;
	return B_OK;
}

status_t GlAlgoNbrWrap::GetInputs(GlAlgoNbrInList& list)
{
	list.MakeEmpty();
	status_t		err = B_OK;
	if (mCache && mSize > 0 && mCache[0]) err = GetInputs(mCache[0], list);
	return err;
}

status_t GlAlgoNbrWrap::Process(GlArrayF& set)
{
	if (mSize < 1) return B_ERROR;
	for (uint32 k = 0; k < mSize; k++) {
		ArpASSERT(mCache[k]);
		mCache[k]->Process(set);
	}
	return B_OK;
}

void GlAlgoNbrWrap::Free()
{
	delete[] mCache;
	mCache = 0;
	mSize = 0;
}

static inline void _add_input(GlAlgo* a, GlAlgoNbrInList& list)
{
	ArpASSERT(a);
	GlAlgoNbr*			nbr = a->AsNbr();
	if (!nbr) return;
	GlAlgoNbrIn*		in = nbr->AsInput();
	if (!in) return;
	list.Add(in);
}

status_t GlAlgoNbrWrap::GetInputs(GlAlgo* a, GlAlgoNbrInList& list)
{
	while (a) {
		_add_input(a, list);
		uint32				c = a->ChainSize();
		for (uint32 k = 0; k < c; k++) {
			GlAlgo*			a2 = a->AlgoAt(k);
			if (a2) {
				status_t	err = GetInputs(a2, list);
				if (err != B_OK) return err;
			}
		}
		a = a->mNext;
	}
	return B_OK;
}

