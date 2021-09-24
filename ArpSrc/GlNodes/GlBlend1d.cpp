#include <ArpCore/StlVector.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlBlend1d.h>

static const int32		GL_BLEND_KEY		= 'A1Bl';

static const int32		_MAP_KEY			= 'map_';
static const int32		_BLEND_KEY			= 'blnd';

/***************************************************************************
 * _GL-BLEND-1D
 ***************************************************************************/
class _GlBlend1d : public GlAlgo1d
{
public:
	_GlBlend1d(gl_node_id nid, GlAlgo1d* blend, std::vector<GlAlgo1d*>& maps);
	_GlBlend1d(const _GlBlend1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

protected:
	virtual uint32		properties() const;

private:
	typedef GlAlgo1d	inherited;
	int32				mBlendIndex;

	float*				Make(const GlAlgo1d* a, int32 size) const;
};

/***************************************************************************
 * GL-BLEND-1D
 ***************************************************************************/
GlBlend1d::GlBlend1d(const GlNode1dAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this, 1), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SAWTOOTH_KEY, 0);
		if (node) chain->AddNode(node);
	}

	chain = VerifyChain(new GlChain(_BLEND_KEY, GL_1D_IO, SZ(SZ_Blend), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SAWTOOTH_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlBlend1d::GlBlend1d(const GlBlend1d& o)
		: inherited(o)
{
}

GlNode* GlBlend1d::Clone() const
{
	return new GlBlend1d(*this);
}

GlAlgo* GlBlend1d::Generate(const gl_generate_args& args) const
{
	/* Since I can have a variable number of chains I'm a little more
	 * complicated than the typical Generate().  Make an algo for each
	 * chain and put it in the correct container.
	 */
	std::vector<GlAlgo1d*>	maps;
	GlAlgo1d*			blend = 0;
	uint32				size = ChainSize();

	for (uint32 k = 0; k < size; k++) {
		const GlChain*	c = ChainAt(k);
		if (c) {
			GlAlgo*		a = c->Generate(args);
			GlAlgo1d*	a1d = (a) ? a->As1d() : 0;
			if (!a1d) delete a;
			else {
				if (c->Key() == _BLEND_KEY) blend = a1d;
				else if (c->Key() == _MAP_KEY) maps.push_back(a1d);
				else delete a;
			}
		}
	}

	return new _GlBlend1d(Id(), blend, maps);
}

// #pragma mark -

/***************************************************************************
 * GL-BLEND-1D-ADD-ON
 ***************************************************************************/
GlBlend1dAddOn::GlBlend1dAddOn()
		: inherited(SZI[SZI_arp], GL_BLEND_KEY, SZ(SZ_1D), SZ(SZ_Blend), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_BLEND_KEY));
}

GlNode* GlBlend1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlBlend1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-BLEND-1D
 ***************************************************************************/
_GlBlend1d::_GlBlend1d(	gl_node_id nid, GlAlgo1d* blend,
						std::vector<GlAlgo1d*>& maps)
		: inherited(GL_BLEND_KEY, nid), mBlendIndex(-1)
{
	if (blend) {
		SetChain(blend, 0);
		mBlendIndex = 0;
	}
	for (uint32 k = 0; k < maps.size(); k++) {
		if (maps[k]) SetChain(maps[k], k + 1);
	}
}

_GlBlend1d::_GlBlend1d(const _GlBlend1d& o)
		: inherited(o), mBlendIndex(o.mBlendIndex)
{
}

GlAlgo* _GlBlend1d::Clone() const
{
	return new _GlBlend1d(*this);
}

status_t _GlBlend1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	std::vector<const GlAlgo1d*>		mapsVec;
	const GlAlgo1d*				blend1d = 0;
	uint32						k;
	for (k = 0; k < ChainSize(); k++) {
		const GlAlgo1d*			a = Algo1dAt(k);
		if (a) {
			if (mBlendIndex == int32(k)) blend1d = a;
			else mapsVec.push_back(a);
		}
	}

	if (!blend1d || mapsVec.size() < 2) {
		if (mapsVec.size() > 0) mapsVec[0]->Run(line, at, size, 0);
		return B_OK;
	}

	float*						blend = new float[size];
	if (!blend) return B_NO_MEMORY;
	if (blend1d->Run(blend, (float*)0, size) != B_OK) {
		delete[] blend;
		return B_ERROR;
	}
	uint32						mapsSize = uint32(mapsVec.size());
	float**						maps = new float*[mapsSize];
	if (!maps) {
		delete[] blend;
		return B_NO_MEMORY;
	}
	for (k = 0; k < mapsSize; k++) maps[k] = 0;

	/* For each step in the line, find the two maps I'm interpolating
	 * between.  If they don't exist, make them.
	 */
	status_t					err = B_OK;
	for (int32 step = 0; step < size; step++) {
		float					b = blend[step] * (mapsSize - 1);
		int32					low = ARP_ROUND(floor(b));
		float					frac = b - low;
		if (low + 1 >= int32(mapsSize)) {
			low = mapsSize - 2;
			frac = 1;
		}
		ArpASSERT(low >= 0 && low + 1 < int32(mapsSize));
		if (low < 0) low = 0;
		else if (low + 1 >= int32(mapsSize)) low = mapsSize - 2;
		ArpASSERT(low >= 0 && low + 1 < int32(mapsSize));
		if (!maps[low] && (maps[low] = Make(mapsVec[low], size)) == 0) {
			err = B_ERROR;
			break;
		}
		if (!maps[low + 1] && (maps[low + 1] = Make(mapsVec[low + 1], size)) == 0) {
			err = B_ERROR;
			break;
		}
		float					f1 = maps[low][step], f2 = maps[low + 1][step];
		float					v = f1 + ((f2 - f1) * (frac));
		line[step] *= arp_clip_1(v);
	}

	delete[] blend;
	for (k = 0; k < mapsSize; k++) delete[] maps[k];
	delete[] maps;

	return err;
}

uint32 _GlBlend1d::properties() const
{
	uint32		ans = 0;
	for (uint32 k = 0; k < ChainSize(); k++) {
		const GlAlgo1d*			a = Algo1dAt(k);
		if (a) ans |= a->Properties();
	}
	ans &= ~CONSTANT_F;
	return ans;
}

float* _GlBlend1d::Make(const GlAlgo1d* a, int32 size) const
{
	ArpVALIDATE(a, return 0);
	float*			ans = new float[size];
	if (!ans) return 0;
	if (a->Run(ans, (float*)0, size) == B_OK) return ans;
	delete[] ans;
	return 0;
}
