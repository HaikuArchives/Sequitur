#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlArrayF.h>
#include <GlNodes/GlMedianNbr.h>

static const int32		GL_MEDIAN_KEY	= '#mdn';

/***************************************************************************
 * _GL-MEDIAN-ALGO
 ***************************************************************************/
class _GlMedianAlgo : public GlAlgoNbr
{
public:
	_GlMedianAlgo(gl_node_id nid);
	_GlMedianAlgo(const _GlMedianAlgo& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(GlArrayF& set);

private:
	typedef GlAlgoNbr	inherited;

	float				_quick_select(float* arr, int n);
};

/***************************************************************************
 * GL-MEDIAN-NBR
 ***************************************************************************/
GlMedianNbr::GlMedianNbr(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlMedianNbr::GlMedianNbr(const GlMedianNbr& o)
		: inherited(o)
{
}

GlNode* GlMedianNbr::Clone() const
{
	return new GlMedianNbr(*this);
}

GlAlgo* GlMedianNbr::Generate(const gl_generate_args& args) const
{
	return new _GlMedianAlgo(Id());
}

// #pragma mark -

/***************************************************************************
 * GL-MEDIAN-NMB-ADD-ON
 ***************************************************************************/
GlMedianNbrAddOn::GlMedianNbrAddOn()
		: inherited(SZI[SZI_arp], GL_MEDIAN_KEY, SZ(SZ_Numbers), SZ(SZ_median), 1, 0)
{
	ArpASSERT(GlAlgoNbr::RegisterKey(GL_MEDIAN_KEY));
}

GlNode* GlMedianNbrAddOn::NewInstance(const BMessage* config) const
{
	return new GlMedianNbr(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-MEDIAN-ALGO
 ***************************************************************************/
_GlMedianAlgo::_GlMedianAlgo(gl_node_id nid)
		: inherited(GL_MEDIAN_KEY, nid)
{
}

_GlMedianAlgo::_GlMedianAlgo(const _GlMedianAlgo& o)
		: inherited(o)
{
}

GlAlgo* _GlMedianAlgo::Clone() const
{
	return new _GlMedianAlgo(*this);
}

status_t _GlMedianAlgo::Process(GlArrayF& set)
{
	if (set.size < 2) return B_OK;
	float		v = _quick_select(set.n, int(set.size));
	set.Resize(1);
	if (set.size > 0) set.n[0] = v;
	return B_OK;
}

#define ELEM_SWAP(a,b) { float t=(a);(a)=(b);(b)=t; }

/* QuickSelect is based on an algorithm from Numerical Recipes in C.
 * Pick it up!
 */
float _GlMedianAlgo::_quick_select(float* arr, int n) 
{
    int		low, high;
    int		median;
    int		middle, ll, hh;

    low = 0;
    high = n-1;
    median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    }
}

#undef ELEM_SWAP
