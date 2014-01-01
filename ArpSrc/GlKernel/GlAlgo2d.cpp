#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlCache2d.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlNodeData.h>
#include <GlPublic/GlPlanes.h>

/***************************************************************************
 * GL-ALGO-2D
 ****************************************************************************/
GlAlgo2d::~GlAlgo2d()
{
}

GlFillType GlAlgo2d::FillType() const
{
	return mFillType;
}

GlAlgo2d* GlAlgo2d::As2d()
{
	return this;
}

const GlAlgo2d* GlAlgo2d::As2d() const
{
	return this;
}

status_t GlAlgo2d::Process(const GlPlanes* src, GlPlanes& dest,
							GlProcessStatus* status)
{
	status_t		err = B_OK;
	for (uint32 k = 0; k < dest.size; k++) {
		if ((err = Process(src, dest.plane[k], dest.w, dest.h, status)) != B_OK) break;
	}
	return err;
}

status_t GlAlgo2d::Process(const GlPlanes* src, uint8* dest, int32 w, int32 h,
							GlProcessStatus* status)
{
	ArpASSERT(false);
	return B_ERROR;
}

/* FIX:  This implementation is temporary.  As soon as
 * all subclasses implement it.
 */
status_t GlAlgo2d::Process(const GlPlanes& src, GlPath& path)
{
	ArpASSERT(false);
	return B_ERROR;
}

status_t GlAlgo2d::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	GlNodeData*			data;
	for (uint32 k = 0; (data = list.DataAt(k)) != 0; k++) {
		if (data->Type() == data->IMAGE_TYPE) {
			GlImage*	img = ((GlNodeDataImage*)data)->Image();
			if (img) {
				status_t	err = PerformImage(img, args);
				if (err != B_OK) {
					list.DeleteContents();
					return err;
				}
			}
		}
	}
	return  B_OK;
}

status_t GlAlgo2d::PerformImage(GlImage* img, const gl_process_args* args)
{
	if (mTargets == 0) return B_OK;
	if (img->InitCheck() != B_OK) return B_ERROR;
	GlPlanes*		pixels = img->LockPixels(mTargets, true);
	ArpASSERT(pixels);
	status_t		err = B_OK;
	if (pixels) err = Process(pixels, *pixels, (args) ? args->status : 0);
	img->UnlockPixels(pixels);
	return err;
}

GlAlgo2d::GlAlgo2d(	gl_node_id nid, uint32 targets,
						GlFillType fillType, int32 token)
		: inherited(nid, token), mTargets(targets),
		  mFillType(fillType)
{
}

GlAlgo2d::GlAlgo2d(const GlAlgo2d& o)
		: inherited(o), mTargets(o.mTargets),
		  mFillType(o.mFillType)
{
}

status_t GlAlgo2d::ProcessAll(	const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status)
{
	GlAlgo*			a = this;
	status_t		err = B_OK;
	while (a) {
		if (a->Io() == GL_2D_IO) {
			if ((err = ((GlAlgo2d*)a)->Process(src, dest, status)) != B_OK) return err;
		}
		a = a->mNext;
	}
	return err;
}

#if 0
status_t GlAlgo2d::ProcessAll(const GlPlanes* src, GlImage& dest)
{
	GlAlgo*			a = this;
	status_t		err = B_OK;
	while (a) {
		if (a->Io() == GL_2D_IO && ((GlAlgo2d*)a)->mTargets > 0) {
			GlPlanes*	planes = dest.LockPixels(mTargets);
			if (planes) err = ((GlAlgo2d*)a)->Process(src, *planes);
			if (err != B_OK) return err;
			dest.UnlockPixels(planes);
		}
		a = a->mNext;
	}
	return err;
}
#endif

status_t GlAlgo2d::ProcessAll(const GlPlanes& src, GlPath& path)
{
	GlAlgo*			a = this;
	status_t		err = B_OK;
	while (a) {
		if (a->Io() == GL_2D_IO) {
			if ((err = ((GlAlgo2d*)a)->Process(src, path)) != B_OK) return err;
		}
		a = a->mNext;
	}
	return err;
}

status_t GlAlgo2d::ProcessAll(const GlPlanes* pixels, uint8* mask, int32 w, int32 h)
{
	GlAlgo*			a = this;
	status_t		err = B_OK;
	while (a) {
		if (a->Io() == GL_2D_IO) {
			if ((err = ((GlAlgo2d*)a)->Process(pixels, mask, w, h)) != B_OK) return err;
		}
		a = a->mNext;
	}
	return err;
}

uint8* GlAlgo2d::NewMask(const GlPlanes& pixels)
{
	ArpVALIDATE(pixels.w > 0 && pixels.h > 0, return 0);
	/* Make a plane to wrap around the mask and send it through the Process()
	 */
	GlPlanes	plane(pixels.w, pixels.h);
	if (plane.SetSize(1) != B_OK) return 0;
	/* Make the mask data.
	 */
	uint8*		mask = new uint8[pixels.w * pixels.h];
	if (!mask) return 0;
	for (int32 pix = 0; pix < pixels.w * pixels.h; pix++) mask[pix] = 255;

	plane.plane[0] = mask;
	ProcessAll(&pixels, plane);
	return mask;
}

// #pragma mark -

/*******************************************************
 * GL-ALGO-2D-WRAP
 *******************************************************/
GlAlgo2dWrap::GlAlgo2dWrap(GlAlgo* a)
		: cache(0), size(0)
{
	if (a) SetAlgo(a);
}

GlAlgo2dWrap::~GlAlgo2dWrap()
{
	Free();
}

status_t GlAlgo2dWrap::InitCheck() const
{
	if (cache && size > 0) return B_OK;
	return B_ERROR;
}

status_t GlAlgo2dWrap::SetAlgo(GlAlgo* algo)
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
	cache = new GlAlgo2d*[c];
	if (!cache) return B_NO_MEMORY;
	
	a = algo;
	cur = 0;
	for (k = 0; k < c; k++) {
		cache[cur] = 0;
		if (a) {
			cache[cur] = a->As2d();
			a = a->mNext;
			if (cache[cur]) cur++;
		}
	}
	size = cur;
	return B_OK;
}

status_t GlAlgo2dWrap::Process(	const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status)
{
	if (size < 1) return B_ERROR;
	status_t		err = B_OK;
	for (uint32 k = 0; k < size; k++) {
		ArpASSERT(cache[k]);
		err = cache[k]->Process(src, dest, status);
		if (err != B_OK) break;
	}
	return err;
}

uint8* GlAlgo2dWrap::Cache(	GlCache2d& cache, const GlPlanes* pixels,
							GlProcessStatus* status)
{
	int32		w = cache.Width(), h = cache.Height();
	if (pixels) { w = pixels->w; h = pixels->h; }

	uint8*		d = cache.SetDimensions(w, h);
	if (!d) return 0;
	
	GlPlanes	dest(w, h);
	if (dest.SetSize(1) != B_OK) return 0;
	// Start at the max
	memset(d, 255, w * h);
	dest.plane[0] = d;
	cache.mStatus = Process(pixels, dest, status);
	if (cache.mStatus == B_OK) return d;
	return 0;
}

void GlAlgo2dWrap::Free()
{
	delete[] cache;
	cache = 0;
	size = 0;
}
