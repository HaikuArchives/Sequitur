#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlFilterKernel.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlEdge.h>

/***************************************************************************
 * GL-EDGE-2D
 ***************************************************************************/
class GlEdge2d : public GlAlgo2d
{
public:
	GlEdge2d(gl_node_id nid, uint32 targets);
	GlEdge2d(const GlEdge2d& o);

	virtual GlAlgo2d*	Clone() const;
	virtual status_t	Process(const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status = 0);
	
private:
	typedef GlAlgo2d	inherited;
	GlFilterKernel		mKernelX, mKernelY;
	int32				mTapsL, mTapsT, mTapsR, mTapsB, mTapsW, mTapsH;
	float*				mTapsX;
	float*				mTapsY;

	void				ProcessLoop(const GlPlanes& src, GlPlanes& dest);

	status_t			Setup();
	void				Cleanup();

	void				InitKernels();
};

/***************************************************************************
 * GL-EDGE
 ***************************************************************************/
GlEdge::GlEdge(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlEdge::GlEdge(const GlEdge& o)
		: inherited(o)
{
}

GlNode* GlEdge::Clone() const
{
	return new GlEdge(*this);
}

GlAlgo* GlEdge::Generate(const gl_generate_args& args) const
{
	return new GlEdge2d(Id(), PixelTargets());
}

// #pragma mark -

/***************************************************************************
 * GL-EDGE-SRF-ADD-ON
 ***************************************************************************/
GlEdgeAddOn::GlEdgeAddOn()
		: inherited(SZI[SZI_arp], 'ApEd', SZ(SZ_Color), SZ(SZ_Edge), 1, 0)
{
}

GlNode* GlEdgeAddOn::NewInstance(const BMessage* config) const
{
	return new GlEdge(this, config);
}

/***************************************************************************
 * GL-EDGE-ALGO
 ***************************************************************************/
GlEdge2d::GlEdge2d(gl_node_id nid, uint32 targets)
		: inherited(nid, targets)
{
	InitKernels();
}

GlEdge2d::GlEdge2d(const GlEdge2d& o)
		: inherited(o), mKernelX(o.mKernelX), mKernelY(o.mKernelY)
{
}

GlAlgo2d* GlEdge2d::Clone() const
{
	return new GlEdge2d(*this);
}

status_t GlEdge2d::Process(	const GlPlanes* src, GlPlanes& dest,
							GlProcessStatus* status)
{
	if (!src) return B_OK;
	if (dest.size < 1) return B_OK;
	ArpVALIDATE(src->b, return B_OK);
	ArpVALIDATE(src->w == dest.w && src->h == dest.h, return B_OK);
	if (Setup() == B_OK) {
		/* Special case -- sometimes the src and dest are the same
		 * object.  Since I destructively write to the dest, I need
		 * to clone the source to work right.
		 */
		if (src != &dest) ProcessLoop(*src, dest);
		else {
			GlPlanes*		cache = src->Clone();
			if (cache && cache->size == src->size) {
// It's cooler without this!
				for (uint32 k = 0; k < dest.size; k++) memset(dest.plane[k], 255, dest.w * dest.h);
				ProcessLoop(*cache, dest);
			}
			delete cache;
		}
	}
	Cleanup();
	return B_OK;
}

void GlEdge2d::ProcessLoop(const GlPlanes& src, GlPlanes& dest)
{
	uint32			k;
	int32			destPix = 0, w = dest.w, h = dest.h;

	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			int32	gradX = 0, gradY = 0;

			for (int32 row = -mTapsT; row <= mTapsB; row++) { 
				for (int32 col = -mTapsL; col <= mTapsR; col++) {
					uint8	intensity = src.b[ARP_PIXEL_SE(x + col, y + row, w, h)];
					gradX += int32(intensity * mTapsX[ARP_PIXEL(col + mTapsL, row + mTapsT, mTapsW)]);
					gradY += int32(intensity * mTapsY[ARP_PIXEL(col + mTapsL, row + mTapsT, mTapsW)]);
				}
			}

			for (k = 0; k < dest.size; k++) {
				uint8	gradMag = arp_clip_255(0.5f + sqrt(float(gradX * gradX + gradY * gradY)));
				if (gradMag < dest.plane[k][destPix]) dest.plane[k][destPix] = gradMag;
			}
			destPix++;
		}
	}
}

status_t GlEdge2d::Setup()
{
	status_t		err;
	if ((err = mKernelX.InitCheck()) != B_OK) return err;
	if ((err = mKernelY.InitCheck()) != B_OK) return err;
	if (mKernelX.Width() != mKernelY.Width()) return B_ERROR;
	if (mKernelX.Height() != mKernelY.Height()) return B_ERROR;
	
	mTapsX = mKernelX.LockTaps(&mTapsW, &mTapsH);
	if (!mTapsX) return B_ERROR;
	mTapsY = mKernelY.LockTaps(&mTapsW, &mTapsH);
	if (!mTapsY) {
		mKernelX.UnlockTaps(mTapsX);
		return B_ERROR;
	}
	mTapsL = mKernelX.Left();
	mTapsT = mKernelX.Top();
	mTapsR = mKernelX.Right();
	mTapsB = mKernelX.Bottom();
	return B_OK;
}

void GlEdge2d::Cleanup()
{
	mKernelX.UnlockTaps(mTapsX);
	mKernelY.UnlockTaps(mTapsY);
}

void GlEdge2d::InitKernels()
{
	mKernelX.Init(1, 1, 1, 1);
	if (mKernelX.InitCheck() != B_OK) return;
	mKernelY.Init(1, 1, 1, 1);
	if (mKernelY.InitCheck() != B_OK) return;
	if (mKernelX.Size() != mKernelY.Size()) return;
	ArpVALIDATE(mKernelX.Size() == 9, return);

	int32		w, h;
	float*		taps = mKernelX.LockTaps(&w, &h);
	if (taps && (w * h == 9)) {
		taps[0] = -1; taps[1] = 0; taps[2] = 1;
		taps[3] = -2; taps[4] = 0; taps[5] = 2;
		taps[6] = -1; taps[7] = 0; taps[8] = 1;
	}
	mKernelX.UnlockTaps(taps);

	taps = mKernelY.LockTaps(&w, &h);
	if (taps && (w * h == 9)) {
		taps[0] = 1; taps[1] = 2; taps[2] = 1;
		taps[3] = 0; taps[4] = 0; taps[5] = 0;
		taps[6] = -1; taps[7] = -2; taps[8] = -1;
	}
	mKernelY.UnlockTaps(taps);

//	mKernelX.Print();
//	mKernelY.Print();
}

