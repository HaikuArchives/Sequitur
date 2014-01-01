#include <stdio.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlFreezeMap.h>

static const int32		GL_FREEZE_KEY	= 'AmFe';
static const int32		_MAP_KEY		= 'map_';
static const int32		_MAP_INDEX		= 0;

enum {
	_FREEZE_MORPH		= 0x00000001,
	_FREEZE_RANDOM		= 0x00000002
};

/***************************************************************************
 * _GL-SCALE-MAP
 ***************************************************************************/
class _GlFreezeMap : public GlAlgo1d
{
public:
	_GlFreezeMap(gl_node_id nid, uint32 flags, GlAlgo* map);
	_GlFreezeMap(const _GlFreezeMap& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

protected:
	virtual uint32		properties() const;

private:
	typedef GlAlgo1d		inherited;
	uint32				mFlags;
};

/***************************************************************************
  * GL-FREEZE-CURVE
 ***************************************************************************/
GlFreezeMap::GlFreezeMap(const GlFreezeMapAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this));
}

GlFreezeMap::GlFreezeMap(const GlFreezeMap& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlFreezeMap::Clone() const
{
	return new GlFreezeMap(*this);
}

GlAlgo* GlFreezeMap::Generate(const gl_generate_args& args) const
{
	GlAlgo*			map = GenerateChainAlgo(_MAP_KEY, args);
	if (!map) return 0;

	uint32			flags = 0;
	if (Params().Bool('mrph', true)) flags |= _FREEZE_MORPH;
	if (Params().Bool('rndm', true)) flags |= _FREEZE_RANDOM;

	return new _GlFreezeMap(Id(), flags, map);
}

// #pragma mark -

/***************************************************************************
 * GL-FREEZE-MAP-ADD-ON
 ***************************************************************************/
GlFreezeMapAddOn::GlFreezeMapAddOn()
		: inherited(SZI[SZI_arp], GL_FREEZE_KEY, SZ(SZ_Wraps), SZ(SZ_Freeze), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_FREEZE_KEY));
	mMorph		= AddParamType(new GlBoolParamType('mrph', SZ(SZ_Freeze_morphing), true));
	mRandom		= AddParamType(new GlBoolParamType('rndm', SZ(SZ_Freeze_random), true));
}

GlNode* GlFreezeMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlFreezeMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SCALE-MAP
 ***************************************************************************/
_GlFreezeMap::_GlFreezeMap(gl_node_id nid, uint32 flags, GlAlgo* map)
		: inherited(GL_FREEZE_KEY, nid), mFlags(flags)
{
	if (map) SetChain(map, _MAP_INDEX);
}

_GlFreezeMap::_GlFreezeMap(const _GlFreezeMap& o)
		: inherited(o), mFlags(o.mFlags)
{
}

GlAlgo* _GlFreezeMap::Clone() const
{
	return new _GlFreezeMap(*this);
}

status_t _GlFreezeMap::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	const GlAlgo1d*		a = Algo1dAt(_MAP_INDEX);
	if (!a) return B_ERROR;
	return a->Run(line, at, size, INIT_LINE_F);
}

uint32 _GlFreezeMap::properties() const
{
	uint32		props = inherited::properties();
	if (mFlags&_FREEZE_MORPH) props &= ~(GlAlgo1d::MORPHING_F);
	if (mFlags&_FREEZE_RANDOM) props &= ~(GlAlgo1d::RANDOM_F);
	return props;
}
