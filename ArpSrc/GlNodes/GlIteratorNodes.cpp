#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlStrainedParamList.h>
#include <GlNodes/GlIteratorNodes.h>

static const int32		GL_ITERATOR_IM_KEY		= 'AiIt';
static const int32		_CHAIN_KEY				= 'chan';
static const int32		_STEP_KEY				= 'step';
static const int32		_STEPS_KEY				= 'stps';

// _GL-ITERATOR-VIEW
class _GlIteratorView : public GlParamView
{
public:
	_GlIteratorView(const gl_new_view_params& params,
					gl_chain_id cid, gl_node_id nid,
					GlStrainedParamList& list);

private:
	typedef GlParamView	inherited;
};

/***************************************************************************
 * GL-ITERATOR-IM
 ****************************************************************************/
class _GlIteratorIm : public GlAlgoIm
{
public:
	_GlIteratorIm(	gl_node_id nid, const GlNode* node,
					const gl_generate_args& args);
	_GlIteratorIm(const _GlIteratorIm& o);

	virtual GlAlgo*		Clone() const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
};

/***************************************************************************
 * GL-ITERATOR-IM
 ***************************************************************************/
GlIteratorIm::GlIteratorIm(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	VerifyChain(new GlChain(_CHAIN_KEY, GL_IMAGE_IO, SZ(SZ_Chain), this, 1));
}

GlIteratorIm::GlIteratorIm(const GlIteratorIm& o)
		: inherited(o)
{
}

GlNode* GlIteratorIm::Clone() const
{
	return new GlIteratorIm(*this);
}

GlAlgo* GlIteratorIm::Generate(const gl_generate_args& args) const
{
	return new _GlIteratorIm(Id(), this, args);
}

BView* GlIteratorIm::NewView(gl_new_view_params& params) const
{
	if (params.viewType != GL_INSPECTOR_VIEW)
		return inherited::NewView(params);

	if (!Parent()) return 0;
	GlStrainedParamList		list;
	if (GetParams(list) != B_OK) return 0;
	return new _GlIteratorView(params, Parent()->Id(), Id(), list);
}

// #pragma mark -

/***************************************************************************
 * GL-ITERATOR-IM-ADD-ON
 ***************************************************************************/
GlIteratorImAddOn::GlIteratorImAddOn()
		: inherited(SZI[SZI_arp], GL_ITERATOR_IM_KEY, SZ(SZ_Flow), SZ(SZ_Iterator), 1, 0)
{
	AddParamType(new GlInt32ParamType(_STEP_KEY, SZ(SZ_Step), 1, 255, 1));
	AddParamType(new GlInt32ParamType(_STEPS_KEY, SZ(SZ_of), 1, 255, 1));
}

GlNode* GlIteratorImAddOn::NewInstance(const BMessage* config) const
{
	return new GlIteratorIm(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-ITERATOR-IM
 ****************************************************************************/
_GlIteratorIm::_GlIteratorIm(	gl_node_id nid, const GlNode* node,
								const gl_generate_args& args)
		: inherited(nid)
{
	uint32				k, size = node->ChainSize(), count = 0;
	for (k = 0; k < size; k++) {
		const GlChain*	c = node->ChainAt(k);
		if (c && c->Key() == _CHAIN_KEY) {
			GlAlgo*		a = c->Generate(args);
			if (a) {
				SetChain(a, count);
				count++;
			}
		}
	}
}

_GlIteratorIm::_GlIteratorIm(const _GlIteratorIm& o)
		: inherited(o)
{
}

GlAlgo* _GlIteratorIm::Clone() const
{
	return new _GlIteratorIm(*this);
}

status_t _GlIteratorIm::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	return B_ERROR;
}

// #pragma mark -

/***************************************************************************
 * _GL-ITERATOR-VIEW
 ****************************************************************************/
_GlIteratorView::_GlIteratorView(	const gl_new_view_params& params,
									gl_chain_id cid, gl_node_id nid,
									GlStrainedParamList& list)
		: inherited(params, cid, nid)
{
	AddParamControls(list);
}
