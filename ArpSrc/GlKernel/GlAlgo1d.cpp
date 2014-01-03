#include <typeinfo>
#include <ArpCore/StlVector.h>
#include <support/Errors.h>
#include <GlPublic/GlActions.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>

static vector<int32>		gKeys;

/***************************************************************************
 * GL-MAP
 ****************************************************************************/
GlAlgo1d::~GlAlgo1d()
{
}

int32 GlAlgo1d::Key() const
{
	return mKey;
}

uint32 GlAlgo1d::Properties(uint32 flags) const
{
	const GlAlgo*			a = this;
	uint32					ans = 0;
	while (a) {
		const GlAlgo1d*		a1d = a->As1d();
		if (a1d) ans |= a1d->properties();
		if (flags&SINGLE_ALGO_F) break;
		a = a->mNext;
	}
	/* Some algos always produce the same output no matter what the input.
	 * They will return an answer with the CONSTANT_F on.  However, I am
	 * only constant if everyone in my chain reported this was true -- and
	 * I just take a shortcut and say I can only have a single algo if I'm
	 * to be constant, just because it doesn't make sense to stack up
	 * multiple constants.
	 */
	if (mNext) ans &= ~CONSTANT_F;
	return ans;
}

GlAlgo1d* GlAlgo1d::As1d()
{
	return this;
}

const GlAlgo1d* GlAlgo1d::As1d() const
{
	return this;
}

	/* Answer the value where at is between 0 and 1.
	 */
#if 0
	float					At(float at) const;
	float					At(float v, float at) const;
#endif

#if 0
float GlAlgo1d::At(float v) const
{
	ArpASSERT(v >= 0 && v <= 1);
	float	atV = v;
	v = 1;
	Run(&v, &atV, 1, 0);
	return v;
}

float GlAlgo1d::At(float v, float at) const
{
	ArpASSERT(v >= 0 && v <= 1);
	ArpASSERT(at >= 0 && at <= 1);
	Run(&v, &at, 1, 0);
	return v;
}
#endif

#if 0
	status_t				Run(float* line, int32 size, uint32 flags = INIT_LINE_F) const;

status_t GlAlgo1d::Run(float* line, int32 size, uint32 flags) const
{
	ArpASSERT(line);
	if (flags&INIT_LINE_F) {
		ArpASSERT(mInitVal >= 0 && mInitVal <= 1);
		for (int32 k = 0; k < size; k++) line[k] = mInitVal;
	}
	
	flags = ALGO_HEAD_F;
	const GlAlgo*				a = this;
	while (a) {
		if (a->Io() == GL_1D_IO) {
			status_t			err = ((GlAlgo1d*)a)->Algo(line, 0, size, flags);
			if (err != B_OK) return err;
		}
		flags = 0;
		a = a->mNext;
	}
	return B_OK;
}
#endif

status_t GlAlgo1d::Run(float* line, float* at, int32 size, uint32 flags) const
{
	ArpASSERT(line);
	bool						single = (flags&SINGLE_ALGO_F) > 0;
	if (flags&INIT_LINE_F) {
		ArpASSERT(mInitVal >= 0 && mInitVal <= 1);
		for (int32 k = 0; k < size; k++) line[k] = mInitVal;
	}
	
	flags = ALGO_HEAD_F;
	const GlAlgo*				a = this;
	while (a) {
		const GlAlgo1d*			a1d = a->As1d();
		if (a1d) {
			status_t			err = a1d->Algo(line, at, size, flags);
			if (err != B_OK) return err;
		}
		if (single) return B_OK;
		flags = 0;
		a = a->mNext;
	}
	return B_OK;
}

#if 0
	/* A convience -- just like the first Run variant but I create the cache.
	 * The client is reponsible for deleting it.
	 */
	float*					Run(int32 size) const;
float* GlAlgo1d::Run(int32 size) const
{
	ArpVALIDATE(size > 0, return 0);
	float*			line = new float[size];
	if (!line) return 0;
	Run(line, size);
	return line;
}
#endif

GlCache1d* GlAlgo1d::NewCache(uint32 size, uint32 frames, uint32 flags)
{
	ArpVALIDATE(size > 0, return 0);
	GlCache1d*		cache = new GlCache1d();
	if (!cache) return 0;
	if (cache->Init(this, size, frames, flags) == B_OK) {
		ArpASSERT(cache->w == size);
		return cache;
	}
	delete cache;
	return 0;
}

status_t GlAlgo1d::Promote2d(	uint8* mask, int32 w, int32 h,
								const GlAlgo1d* y1d) const
{
	return B_ERROR;
}

status_t GlAlgo1d::Volume2d(uint8* depth, uint8* mask, int32 w, int32 h,
							uint32 mode, uint32 op,
							const GlAlgo1d* y1d, const GlAlgo1d* z1d) const
{
	return B_ERROR;
}

#if 0
	bool					Init(GlCache1d* cache) const;
bool GlAlgo1d::Init(GlCache1d* cache) const
{
	ArpVALIDATE(cache, return false);
	ArpASSERT(mInitVal >= 0 && mInitVal <= 1);
	uint32			size = cache->w * cache->h;
	for (uint32 k = 0; k < size; k++) cache->n[k] = mInitVal;
	return true;
}
#endif

GlAlgo1d::GlAlgo1d(int32 key, gl_node_id nid, int32 token, float initVal)
		: inherited(nid, token), mKey(key), mInitVal(arp_clip_1(initVal))
{
}

GlAlgo1d::GlAlgo1d(const GlAlgo1d& o)
		: inherited(o), mKey(o.mKey), mInitVal(o.mInitVal)
{
}

uint32 GlAlgo1d::properties() const
{
	uint32					props = 0,
							size = ChainSize();
	for (uint32 k = 0; k < size; k++) {
		const GlAlgo1d*		m = Algo1dAt(k);
		if (m) props |= m->Properties();
	}
	return props;
}

status_t GlAlgo1d::ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const
{
	return B_ERROR;
}

void GlAlgo1d::_print() const
{
	printf("%s key %ld", typeid(*this).name(), mKey);
}

#if 0
	/* Generate a new cache from every algo in my chain.  This is a
	 * special method to operate on heterogenous GlAlgo chains -- since
	 * an algo can report an Io() of GL_1D_IO but not actually be a
	 * subclass of GlAlgo1d, I need to do a little extra work.  This
	 * also means clients might not know what type of object they have.
	 */
	static GlCache1d*	NewCache(GlAlgo* algo, uint32 size, uint32 frames = 0);

GlCache1d* GlAlgo1d::NewCache(GlAlgo* algo, uint32 size, uint32 frames)
{
	ArpVALIDATE(size > 0, return 0);
	ArpVALIDATE(algo, return 0);
	GlCache1d*		cache = new GlCache1d();
	if (!cache) return 0;
	if (cache->NewInit(algo, size, frames) == B_OK) {
		ArpASSERT(cache->w == size);
		return cache;
	}
	delete cache;
	return 0;
}
#endif

bool GlAlgo1d::RegisterKey(int32 key)
{
	for (uint32 k = 0; k < gKeys.size(); k++) {
		if (key == gKeys[k]) return false;
	}
	gKeys.push_back(key);
	return true;
}

// #pragma mark -

/*******************************************************
 * GL-ALGO-1D-WRAP
 *******************************************************/
GlAlgo1dWrap::GlAlgo1dWrap(GlAlgo* a)
		: cache(0), size(0)
{
	if (a) SetAlgo(a);
}

GlAlgo1dWrap::~GlAlgo1dWrap()
{
	Free();
}

status_t GlAlgo1dWrap::InitCheck() const
{
	if (cache && size > 0) return B_OK;
	return B_ERROR;
}

status_t GlAlgo1dWrap::SetAlgo(GlAlgo* algo)
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
	cache = new GlAlgo1d*[c];
	if (!cache) return B_NO_MEMORY;
	
	a = algo;
	cur = 0;
	for (k = 0; k < c; k++) {
		cache[cur] = 0;
		if (a) {
			cache[cur] = a->As1d();
			a = a->mNext;
			if (cache[cur]) cur++;
		}
	}
	size = cur;
	return B_OK;
}

status_t GlAlgo1dWrap::Process(float* line, float* at, int32 lineSize, uint32 flags) const
{
	ArpASSERT(line && lineSize > 0);
	if (size < 1) return B_ERROR;
	bool				single = (flags&GlAlgo1d::SINGLE_ALGO_F) > 0;
	if (flags&GlAlgo1d::INIT_LINE_F) {
		float			init = cache[0]->mInitVal;
		ArpASSERT(init >= 0 && init <= 1);
		for (int32 k = 0; k < lineSize; k++) line[k] = init;
	}
	
	flags = GlAlgo1d::ALGO_HEAD_F;
	for (uint32 k = 0; k < size; k++) {
		ArpASSERT(cache[k]);
		status_t		err = cache[k]->Algo(line, at, lineSize, flags);
		if (err != B_OK) return err;
		if (single) return B_OK;
		flags = 0;
	}
	return B_OK;
}

void GlAlgo1dWrap::Free()
{
	delete[] cache;
	cache = 0;
	size = 0;
}
