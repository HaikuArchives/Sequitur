#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlNodes/GlInvert1d.h>

static const int32		GL_INVERT1D_KEY		= 'A1Iv';

/***************************************************************************
 * _GL-INVERT-1D
 ***************************************************************************/
class _GlInvert1d : public GlAlgo1d
{
public:
	_GlInvert1d(gl_node_id nid);
	_GlInvert1d(const _GlInvert1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

private:
	typedef GlAlgo1d	inherited;
};

/***************************************************************************
  * GL-INVERT-1D
 ***************************************************************************/
GlInvert1d::GlInvert1d(const GlNode1dAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlInvert1d::GlInvert1d(const GlInvert1d& o)
		: inherited(o)
{
}

GlNode* GlInvert1d::Clone() const
{
	return new GlInvert1d(*this);
}

GlAlgo* GlInvert1d::Generate(const gl_generate_args& args) const
{
	return new _GlInvert1d(Id());
}

// #pragma mark -

/***************************************************************************
 * GL-INVERT-1D-ADD-ON
 ***************************************************************************/
GlInvert1dAddOn::GlInvert1dAddOn()
		: inherited(SZI[SZI_arp], GL_INVERT1D_KEY, SZ(SZ_1D), SZ(SZ_Invert), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_INVERT1D_KEY));
}

GlNode* GlInvert1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlInvert1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-INVERT-1D
 ***************************************************************************/
_GlInvert1d::_GlInvert1d(gl_node_id nid)
		: inherited(GL_INVERT1D_KEY, nid)
{
}

_GlInvert1d::_GlInvert1d(const _GlInvert1d& o)
		: inherited(o)
{
}

GlAlgo* _GlInvert1d::Clone() const
{
	return new _GlInvert1d(*this);
}

status_t _GlInvert1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	for (int32 step = 0; step < size; step++) {
		line[step] = 1 - line[step];
	}
	return B_OK;
}
