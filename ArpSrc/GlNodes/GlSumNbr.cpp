#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlArrayF.h>
#include <GlNodes/GlSumNbr.h>

/***************************************************************************
 * _GL-SUM-ALGO
 ***************************************************************************/
class _GlSumAlgo : public GlAlgoNbr
{
public:
	_GlSumAlgo(gl_node_id nid);
	_GlSumAlgo(const _GlSumAlgo& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(GlArrayF& set);

private:
	typedef GlAlgoNbr	inherited;
};

/***************************************************************************
 * GL-SUM-NBR
 ***************************************************************************/
GlSumNbr::GlSumNbr(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlSumNbr::GlSumNbr(const GlSumNbr& o)
		: inherited(o)
{
}

GlNode* GlSumNbr::Clone() const
{
	return new GlSumNbr(*this);
}

GlAlgo* GlSumNbr::Generate(const gl_generate_args& args) const
{
	return new _GlSumAlgo(Id());
}

// #pragma mark -

/***************************************************************************
 * GL-SUM-NBR-ADD-ON
 ***************************************************************************/
GlSumNbrAddOn::GlSumNbrAddOn()
		: inherited(SZI[SZI_arp], GL_SUM_KEY, SZ(SZ_Numbers), SZ(SZ_sum), 1, 0)
{
	ArpASSERT(GlAlgoNbr::RegisterKey(GL_SUM_KEY));
}

GlNode* GlSumNbrAddOn::NewInstance(const BMessage* config) const
{
	return new GlSumNbr(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SUM-ALGO
 ***************************************************************************/
_GlSumAlgo::_GlSumAlgo(gl_node_id nid)
		: inherited(GL_SUM_KEY, nid)
{
}

_GlSumAlgo::_GlSumAlgo(const _GlSumAlgo& o)
		: inherited(o)
{
}

GlAlgo* _GlSumAlgo::Clone() const
{
	return new _GlSumAlgo(*this);
}

status_t _GlSumAlgo::Process(GlArrayF& set)
{
	float		s = 0;
	for (uint32 k = 0; k < set.size; k++) s += set.n[k];
	if (set.Resize(1) != B_OK) return B_NO_MEMORY;
	set.n[0] = s;
	return B_OK;
}
