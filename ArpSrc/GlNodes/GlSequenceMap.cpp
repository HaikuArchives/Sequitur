#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlGlobalsI.h>
#include "GlPublic/GlParam.h"
#include "GlPublic/GlParamList.h"
#include "GlPublic/GlParamType.h"
#include <GlNodes/GlSequenceMap.h>

static const int32		GL_SEQUENCE_KEY		= 'AmSQ';
static const int32		_MAP_KEY			= 'map_';
static const uint32		_MAP_INDEX			= 0;
static const int32		_INIT_RES			= 100;
static const uint32		_INIT_INDEX			= 0;

/*******************************************************
 * _GL-SEQUENCE-ALGO
 *******************************************************/
class _GlSequenceAlgo : public GlAlgo1d
{
public:
	_GlSequenceAlgo(gl_node_id nid, GlAlgo* init,
					const GlMapList& maps);
	_GlSequenceAlgo(const _GlSequenceAlgo& o);
	virtual ~_GlSequenceAlgo();

	virtual GlAlgo*		Clone() const;
	virtual uint32		Flags() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;
	virtual status_t	ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const;

	virtual void		UpdateSource(GlNode* node);

protected:
	virtual bool		set_step(float v);
	/* Override the walking method to walk my sequences
	 * -- if there's a current frame, walk the appropriate
	 * frame map.  If there's not, walk the chain map.
	 * This is intended so the realtime param list can set
	 * params transparently -- I have NO CLUE what sort of
	 * side effects might crop up.
	 */
	virtual int32		walk(GlAlgoAction& action, uint32 io);

	virtual uint32		properties() const;

private:
	typedef GlAlgo1d	inherited;
	GlMapList			mMaps;
	int32				mFrame;

	mutable float*		lineCache;
	mutable float*		atCache;
	mutable int32		sizeCache;

	status_t			MakeCache(float* line, float* at, int32 size) const;
	void				FreeCache() const;
	
protected:
	virtual void		_print() const;
};

/***************************************************************************
 * GL-SEQUENCE-MAP
 ***************************************************************************/
GlSequenceMap::GlSequenceMap(	const GlSequenceMapAddOn* addon,
								const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SINE_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlSequenceMap::GlSequenceMap(const GlSequenceMap& o)
		: inherited(o), mAddOn(o.mAddOn), mMaps(o.mMaps)
{
}

GlNode* GlSequenceMap::Clone() const
{
	return new GlSequenceMap(*this);
}

status_t GlSequenceMap::SetMapParam(const GlAlgo1d* map, int32 index,
									const char* name)
{
	ArpVALIDATE(index >= 0, return B_ERROR);
	if (map == 0) {
		if (index < int32(mMaps.size)) {
			delete mMaps.maps[index];
			mMaps.maps[index] = 0;
		}
		return B_OK;
	}
	GlAlgo1d*			m = (GlAlgo1d*)(map->Clone());
	if (!m) return B_NO_MEMORY;
	status_t			err = mMaps.Set(index, m);
	if (err != B_OK) delete m;
	return err;
}

GlAlgo* GlSequenceMap::Generate(const gl_generate_args& args) const
{
	ArpVALIDATE(mAddOn, return 0);
	int32				frames = Params().Int32('frms');
	if (frames <= 0) return 0;
	GlAlgo*				init = GenerateChainAlgo(_MAP_KEY, args);
	if (!init) return 0;
	if (frames != int32(mMaps.size)) mMaps.ResizeTo(frames, mMaps.RELATIVE_METHOD);
	/* FIX:  Somehow need to track structure changes inside of me,
	 * and wipe out all my maps when one happens.
	 */
	 
	return new _GlSequenceAlgo(Id(), init, mMaps);
}

// #pragma mark -

/***************************************************************************
 * GL-SEQUENCE-MAP-ADD-ON
 ***************************************************************************/
GlSequenceMapAddOn::GlSequenceMapAddOn()
		: inherited(SZI[SZI_arp], GL_SEQUENCE_KEY, SZ(SZ_Recorders), SZ(SZ_Sequence), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_SEQUENCE_KEY));
	mFrames		= AddParamType(new GlInt32ParamType('frms', SZ(SZ_Frames), 1, 2000, 100));
}

GlNode* GlSequenceMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlSequenceMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SEQUENCE-ALGO
 ****************************************************************************/
_GlSequenceAlgo::_GlSequenceAlgo(	gl_node_id nid, GlAlgo* init,
									const GlMapList& maps)
		: inherited(GL_SEQUENCE_KEY, nid), mMaps(maps), mFrame(-1),
		  lineCache(0), atCache(0), sizeCache(0)
{
	if (init) SetChain(init, _MAP_INDEX);
}

_GlSequenceAlgo::_GlSequenceAlgo(const _GlSequenceAlgo& o)
		: inherited(o), mMaps(o.mMaps), mFrame(o.mFrame),
		  lineCache(0), atCache(0), sizeCache(0)
{
}

_GlSequenceAlgo::~_GlSequenceAlgo()
{
	FreeCache();
}

GlAlgo* _GlSequenceAlgo::Clone() const
{
	return new _GlSequenceAlgo(*this);
}

uint32 _GlSequenceAlgo::Flags() const
{
	return REDIRECTOR_F;
}

static inline const GlAlgo1d* _get_map(const GlMapList& list, int32 frame,
									int32 delta, int32* count)
{
	(*count)++;
	frame += delta;
	while (frame >= 0 && frame < int32(list.size)) {
		if (list.maps[frame]) return list.maps[frame];
		(*count)++;
		frame += delta;
	}
	return 0;
}

status_t _GlSequenceAlgo::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	const GlAlgo1d*		init = Algo1dAt(_INIT_INDEX);
	ArpVALIDATE(init, return B_ERROR);
	
	if (mFrame < 0 || mFrame >= int32(mMaps.size))
		return init->Algo(line, at, size, flags);

	if (mMaps.maps[mFrame]) {
		return mMaps.maps[mFrame]->Algo(line, at, size, flags);
	}

	int32				prevCount = 0, nextCount = 0;
	const GlAlgo1d*		prev = _get_map(mMaps, mFrame, -1, &prevCount);
	const GlAlgo1d*		next = _get_map(mMaps, mFrame, 1, &nextCount);

	if (!prev) prev = init;
	if (!next) next = init;

	status_t			err = MakeCache(line, at, size);
	if (err != B_OK) return err;

	if ((err = prev->Algo(line, at, size, flags)) != B_OK) return err;
	if ((err = prev->Algo(lineCache, atCache, size, flags)) != B_OK) return err;

	float				f = prevCount / float(prevCount + nextCount);
	ArpASSERT(f >= 0 && f <= 1.0);
	for (int32 k = 0; k < size; k++)
		line[k] = line[k] + ((lineCache[k] - line[k]) * f);

	return B_OK;
}

void _GlSequenceAlgo::UpdateSource(GlNode* node)
{
	ArpASSERT(node && node->AddOn() && node->AddOn()->Key() == GL_SEQUENCE_KEY);
	for (uint32 k = 0; k < mMaps.size; k++)
		node->SetMapParam(mMaps.maps[k], k, 0);
}

bool _GlSequenceAlgo::set_step(float v)
{
	mFrame = -1;
	if (mMaps.size == 1) mFrame = 0;
	else if (mMaps.size > 1) mFrame = int32(ARP_ROUND(v * (mMaps.size - 1)));
	return true;
}

int32 _GlSequenceAlgo::walk(GlAlgoAction& action, uint32 io)
{
	if (mFrame < 0 || mFrame >= int32(mMaps.size)) return inherited::walk(action, io);
	/* FIX?  Might want to have a mode for whether it creates or not.
	 * Hell, might even want a mode for whether it does the
	 * inherited or this.
	 */
	if (!mMaps.maps[mFrame]) {
		const GlAlgo1d*			init = Algo1dAt(0);
		if (!init) return GL_CONTINUE;
		mMaps.maps[mFrame] = (GlAlgo1d*)(init->Clone());
		if (!mMaps.maps[mFrame]) return GL_CONTINUE;
	}	
	return mMaps.maps[mFrame]->Walk(action, io);
}

uint32 _GlSequenceAlgo::properties() const
{
	return inherited::properties() | MORPHING_F | APPLY_TO_2D_F;
}

status_t _GlSequenceAlgo::ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const
{
	ArpASSERT(false);
	/* FIX:  Can't take the default -- waaaaay too slow.  But
	 * what should this do?
	 */
	return B_OK;
}

status_t _GlSequenceAlgo::MakeCache(float* line, float* at, int32 size) const
{
	if (!line) return B_ERROR;
	if (size > sizeCache) FreeCache();
	
	if (!lineCache) {
		lineCache = new float[size];
		if (!lineCache) return B_NO_MEMORY;
	}
	memcpy(lineCache, line, size * 4);

	if (at) {
		if (!atCache) {
			atCache = new float[size];
			if (!atCache) return B_NO_MEMORY;
		}
		memcpy(atCache, at, size * 4);
	}

	sizeCache = size;
	return B_OK;
}

void _GlSequenceAlgo::FreeCache() const
{
	delete[] lineCache;
	delete[] atCache;
	lineCache = atCache = 0;
	sizeCache = 0;
}

void _GlSequenceAlgo::_print() const
{
	printf("_GlSequenceAlgo key: %ld size %ld\n", mKey, mMaps.size);
#if 0
	for (uint32 k = 0; k < mMaps.size; k++) {
		if (mMaps.maps[k]) {
			for (t = 0; t < tabs + 1; t++) printf("\t");
			printf("%ld: ", k);
			mMaps.maps[k]->Print();
		}
	}
#endif
}
