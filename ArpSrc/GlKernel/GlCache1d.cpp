#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>

/***************************************************************************
  * GL-CACHE-ID
 ***************************************************************************/
GlCache1d::GlCache1d()
		: n(0), w(0), h(0), mProperties(0), mOwner(0)
{
}

GlCache1d::GlCache1d(const GlCache1d& o)
		: n(0), w(0), h(0), mProperties(o.mProperties), mOwner(0)
{
	if (o.w > 0 && o.h > 0) {
		n = new float[o.w * o.h];
		if (!n) return;
		memcpy(n, o.n, (o.w * o.h) * sizeof(float));
		w = o.w;
		h = o.h;
	}
}

GlCache1d::~GlCache1d()
{
	Free();
}

GlCache1d* GlCache1d::Clone() const
{
	return new GlCache1d(*this);
}

#if 0
	status_t		NewInit(GlAlgo* algo, uint32 size, uint32 frames);

static uint32 _get_props(GlAlgo* a)
{
	uint32		props = 0;
	while (a) {
		GlAlgo1d*	a1d = a->As1d();
		if (a1d) props |= a1d->properties();
		a = a->mNext;
	}
	return props;
}

status_t GlCache1d::NewInit(GlAlgo* algo, uint32 s, uint32 frames)
{
	ArpVALIDATE(algo, return B_ERROR);
	uint32			props = _get_props(algo);

	/* If the algo doesn't need frames but they were supplied,
	 * turn them off.
	 */
	if (!(props&GlAlgo1d::MORPHING_F) && !(props&GlAlgo1d::RANDOM_F))
		frames = 0;

	/* Determine the new size of the array.
	 */	
	uint32			height = frames + 1;
	uint32			size = s * height;
	if (size < 1) return B_ERROR;
	
	/* Allocate the array.
	 */
	if (w != s || h != height) {
		Free();
		n = new float[size];
		if (!n) return B_NO_MEMORY;
		w = s;
		h = height;
	}
	ArpASSERT(w == s && h == height);

	mOwner = (void*)algo;
	mProperties = props;

	/* Run each frame.  Only set the first step if I have
	 * multiple frames -- there is a subtlety here: sometimes
	 * the cache is created in a const algo, which is fine as
	 * long as there were no frames.  So I assume if I've got
	 * frames I was created non-const, and otherwise I just play
	 * it safe and don't cause any writes to the algo.
	 */
	if (frames > 0) algo->SetStep(0.0);
	float*				c = n;
	bool				inited = false;
	for (uint32 k = 0; k < h; k++) {
		uint32			flags = GlAlgo1d::ALGO_HEAD_F;
		GlAlgo*			a = algo;
		/* I have to step through each algo in the chain and
		 * find which ones are actually algo1ds.  Since it's
		 * permissable for an algo to report an Io() of
		 * GL_1D_IO but not be a GlAlgo1d subclass, casting
		 * can't be done.
		 */
		while (a) {
			GlAlgo1d*	a1d = a->As1d();
			if (a1d) {
				if (!inited) inited = a1d->Init(this);
				a1d->Algo(c, 0, w, flags);
				flags = 0;
			}
			a = a->mNext;
		}
//		algo->Run(c, (float*)0, w);
		algo->SetStep(k / float(h-1));
		c += w;
	}
	return B_OK;
}
#endif

status_t GlCache1d::Init(GlAlgo1d* algo, uint32 s, uint32 frames, uint32 flags)
{
	ArpVALIDATE(algo, return B_ERROR);
	uint32			props = algo->Properties();

	/* If the algo doesn't need frames but they were supplied,
	 * turn them off.
	 */
	if (!(props&algo->MORPHING_F) && !(props&algo->RANDOM_F))
		frames = 0;

	/* Determine the new size of the array.
	 */	
	uint32			height = frames + 1;
	uint32			size = s * height;
	if (size < 1) return B_ERROR;
	
	/* Allocate the array.
	 */
	if (w != s || h != height) {
		Free();
		n = new float[size];
		if (!n) return B_NO_MEMORY;
		w = s;
		h = height;
	}
	ArpASSERT(w == s && h == height);

	mOwner = (void*)algo;
	mProperties = props;

	/* Run each frame.  Only set the first step if I have
	 * multiple frames -- there is a subtlety here: sometimes
	 * the cache is created in a const algo, which is fine as
	 * long as there were no frames.  So I assume if I've got
	 * frames I was created non-const, and otherwise I just play
	 * it safe and don't cause any writes to the algo.
	 */
	if (frames > 0) algo->SetStep(0.0);
	float*			c = n;
	for (uint32 k = 0; k < h; k++) {
		algo->Run(c, 0, w, flags);
		algo->SetStep(k / float(h-1));
		c += w;
	}
	return B_OK;
}

status_t GlCache1d::SetStep(GlAlgo1d* algo, float step)
{
	ArpVALIDATE(algo, return B_ERROR);
	ArpASSERT(mOwner == (void*)algo);

	if (!n) return B_ERROR;
	if (h > 1) return B_OK;
	if (!(mProperties&algo->MORPHING_F) && !(mProperties&algo->RANDOM_F))
		return B_OK;

	if (algo->SetStep(step) == false) return B_OK;
	return algo->Run(n, 0, w);
}

float GlCache1d::At(float x) const
{
	return Interpolate(x);
}

float GlCache1d::At(float x, float y) const
{
	ArpASSERT(x >= 0 && x <= 1);
	ArpASSERT(y >= 0 && y <= 1);
	if (h <= 1) return Interpolate(x);
	/* 2 point interpolation -- get an interpolated
	 * value for each frame, then interpolate between them.
	 */
	y = y * (h - 1);
	int32			low = ARP_ROUND(floor(y));
	/* If this is the last frame, don't bother with
	 * 2 point interpolation.
	 */
	if (low >= int32(h - 1)) return Interpolate(x, h - 1);
	float			frac = y - low;
	ArpASSERT(low >= 0 && low + 1 < int32(h));
	if (low < 0) low = 0;
	else if (low + 1 >= int32(h)) low = h - 2;
	/* Now I have the height values, so get the width...
	 */
	float			frame0 = Interpolate(x, uint32(low)),
					frame1 = Interpolate(x, uint32(low + 1));
	/* And interpolate...
	 */
	return frame0 + ((frame1 - frame0) * frac);
}

float GlCache1d::Interpolate(float x, uint32 frame) const
{
	ArpASSERT(x >= 0 && x <= 1);
	if (w < 1 || h < 1) return 0.0;
	ArpASSERT(frame < h);
	if (w == 1) return n[0];
	if (x <= 0.0) return n[0];
	if (x >= 1.0) return n[w - 1];
	
	x = x * (w - 1);
	int32			low = ARP_ROUND(floor(x));
	float			frac = x - low;
	ArpASSERT(low >= 0 && low + 1 < int32(w));
	if (low < 0) low = 0;
	else if (low + 1 >= int32(w)) low = w - 2;
	// frame handling -- everything else is identical to the original At()
	low += (frame * w);
	ArpASSERT(low + 1 < int32(w * h));
	// end frame handling
	return n[low] + ((n[low + 1] - n[low]) * frac);
}

void GlCache1d::Free()
{
	delete[] n;
	w = h = 0;
	mProperties = 0;
	mOwner = 0;
}

// #pragma mark -


status_t GlCache1d::Render(	GlAlgo1d* a1d, GlImage* image, GlRect algo, GlRect area,
							uint8 alpha, uint32 flags)
{
	ArpVALIDATE(image, return B_ERROR);
	status_t			err = B_ERROR;
	GlPlanes*			p = image->LockPixels(GL_PIXEL_RGBA);
	if (p && p->HasColor()) err = Render(a1d, p->r, p->g, p->b, p->a, p->w, p->h, algo, area, alpha, flags);
	image->UnlockPixels(p);
	return err;
}

static inline void _render_sides(	uint8* r, uint8* g, uint8* b, uint8* a,
									int32 w, int32 h, int32 x, int32 y, int32 curY,
									const GlRect& area, const ArpVoxel& bgC,
									const ArpVoxel& lineC)
{
	if (x == area.l || x == area.r - 1) {
		int32		pix = ARP_PIXEL(x, curY, w);
		for (y = curY + 1; y < area.b; y++) {
			r[pix] = lineC.r;
			g[pix] = lineC.g;
			b[pix] = lineC.b;
			a[pix] = lineC.a;
			pix += w;
		}
		if (curY < area.b) {
			r[pix] = arp_clip_255((lineC.r + bgC.r) / 2);
			g[pix] = arp_clip_255((lineC.g + bgC.g) / 2);
			b[pix] = arp_clip_255((lineC.b + bgC.b) / 2);
			a[pix] = arp_clip_255((lineC.a + bgC.a) / 2);
		}
	} else {
		int32		pix = ARP_PIXEL(x, area.b - 1, w);
		r[pix] = lineC.r;
		g[pix] = lineC.g;
		b[pix] = lineC.b;
		a[pix] = lineC.a;
	}
}

static inline void _render_line(int32 pix, int32 w, int32 h, int32 offset2,
								uint8* r, uint8* g, uint8* b, uint8* a,
								uint8* grad, const ArpVoxel& lineC)
{
	for (int32 y = 0; y < h; y++) {
		if (grad[offset2 + y] == 255) {
			r[pix] = lineC.r;
			g[pix] = lineC.g;
			b[pix] = lineC.b;
			a[pix] = lineC.a;
		} else if (grad[offset2 + y] > 0) {
			int32	v = grad[offset2 + y];
			r[pix] = arp_clip_255( ((r[pix] * (255 - v)) + (lineC.r * v)) / 255);
			g[pix] = arp_clip_255( ((g[pix] * (255 - v)) + (lineC.g * v)) / 255);
			b[pix] = arp_clip_255( ((b[pix] * (255 - v)) + (lineC.b * v)) / 255);
			a[pix] = arp_clip_255( ((a[pix] * (255 - v)) + (lineC.a * v)) / 255);
		}
		pix += w;
	}
}

static inline void _render_alpha_line(	int32 pix, int32 w, int32 h, int32 offset2,
										uint8* r, uint8* g, uint8* b, uint8* a,
										uint8* grad, const ArpVoxel& lineC)
{
	float			mix = glTable256[lineC.a];
	for (int32 y = 0; y < h; y++) {
		if (grad[offset2 + y] == 255) {
			r[pix] = arp_clip_255(r[pix] + ((int16(lineC.r) - r[pix]) * mix));
			g[pix] = arp_clip_255(g[pix] + ((int16(lineC.g) - g[pix]) * mix));
			b[pix] = arp_clip_255(b[pix] + ((int16(lineC.b) - b[pix]) * mix));
			if (lineC.a > a[pix]) a[pix] = lineC.a;
		} else if (grad[offset2 + y] > 0) {
			float	v = grad[offset2 + y] * mix;
			r[pix] = arp_clip_255( ((r[pix] * (255 - v)) + (lineC.r * v)) / 255);
			g[pix] = arp_clip_255( ((g[pix] * (255 - v)) + (lineC.g * v)) / 255);
			b[pix] = arp_clip_255( ((b[pix] * (255 - v)) + (lineC.b * v)) / 255);
			a[pix] = arp_clip_255( ((a[pix] * (255 - v)) + (lineC.a * v)) / 255);
		}
		pix += w;
	}
}

/* This algo has become a little hacked up as I've added on to it.  The
 * biggest thing is the addition of the alpha arg -- if it's 255, the render
 * works as it always did, just drawing the algo.  If it's anything less,
 * then it assumes we're compositing the algo on another and just draws a
 * green line, no filling.
 */
status_t GlCache1d::Render(	GlAlgo1d* a1d, uint8* r, uint8* g, uint8* b, uint8* a, int32 w, int32 h,
							GlRect algo, GlRect area, uint8 alpha, uint32 flags)
{
	ArpVALIDATE(r && g && b && w > 0 && h > 0 && algo.r - algo.l > 0, return B_ERROR);
	area.ConformTo(GlRect(0, 0, w, h));
	/* The algo must stretch over the area I'm drawing.
	 */
	ArpVALIDATE(algo.l <= area.l && algo.r >= area.r, return B_ERROR);
	ArpVALIDATE(algo.t <= area.t && algo.b >= area.b, return B_ERROR);
	int32				size = (algo.r - algo.l) + 1;
	/* It's OK to take the const off because the cache only needs
	 * to a non-const algo if it has any frames.
	 */
	status_t			err = Init(a1d, size, 0, flags);
	if (err != B_OK) return err;
	uint8*				grad = new uint8[h * 2];
	if (!grad) return B_NO_MEMORY;
	ArpVoxel			bgC(0, 0, 0, 0, 0),
						fgC(0, 0, 100, 100, 0),
						lineC(0, 0, 0, 255, 0);
	/* As a performance thing, I do a blank memset to fill the background,
	 * even though this func lets you render to only part of a source image.
	 * Obviously, I'm assuming this is always the first thing done to an
	 * image.  Right now it's fine, but if at some point a client shows up
	 * that has info in the image, this has to change.
	 */
	if (alpha == 255) {
		memset(r, bgC.r,	w * h);
		memset(g, bgC.g,	w * h);
		memset(b, bgC.b,	w * h);
		memset(a, bgC.a,	w * h);
	} else {
		lineC.r = lineC.b = 100;
		lineC.g = 255;
		lineC.a = alpha;
	}
	
	memset(grad, 0, h);

	int32				y, pix, algoH = algo.b - algo.t;
	ArpASSERT(area.l - algo.l >= 0 && area.l - algo.l < size);
	float				curV = n[area.l - algo.l];
	float				nextV = curV;
	ArpASSERT(curV >= 0 && curV <= 1);
	int32				offset1 = 0, offset2 = h, tmpOffset;
	for (int32 x = area.l; x < area.r; x++) {
		/* Flush out the line once I've done at least 2 steps.
		 */
		if (x > area.l) {
			pix = ARP_PIXEL(x - 1, 0, w);
			if (alpha == 255) _render_line(pix, w, h, offset2, r, g, b, a, grad, lineC);
			else _render_alpha_line(pix, w, h, offset2, r, g, b, a, grad, lineC);
		}

		memset(grad + offset2, 0, h);

		if (x + 1 < area.r) {
			ArpASSERT((x+1) - algo.l >= 0 && (x+1) - algo.l < size);
			nextV = n[(x+1) - algo.l];
			ArpASSERT(nextV >= 0 && nextV <= 1);
		}

		int32			curY = algoH - 1 - int32(curV * algoH);
		if (curY < 0) curY = 0;
		int32			nextY = algoH - 1 - int32(nextV * algoH);
		if (nextY < 0) nextY = 0;

		/* Fill in the foreground
		 */
		if (alpha == 255) {
			for (y = curY; y < area.b; y++) {
				pix = ARP_PIXEL(x, y, w);
				r[pix] = fgC.r;
				g[pix] = fgC.g;
				b[pix] = fgC.b;
				a[pix] = fgC.a;
			}
		}
		
		/* Figure in the gradual line change.  The gradual change, being
		 * an antialiased line, is two pixels wide.  I do this in two
		 * passes, once drawing a line from the cur to the next, once
		 * from the next to the cur.  What complicates this is that I
		 * deal with cases where 3 points are writing to the same vertical
		 * strip of pixels.
		 */
		if (curY != nextY) {
			float		gradRange = float(ARP_ABS(curY - nextY) + 1);
			float		gradStep = 1 / gradRange;
			float		gradCount = gradStep;
			int32		sign = (curY <= nextY) ? 1 : -1;

			/* This draws the offset2 line -- replace whatever's
			 * there since this strip is newly exposed.
			 */
			for (y = curY + sign; y != nextY; y += sign) {
				grad[offset2 + y] = arp_clip_255(gradCount * 255);
				gradCount += gradStep;
			}
			/* This draws the offset1 line -- someone's already
			 * written to it, so only go higher.
			 */
			gradCount = gradStep;
			for (y = nextY; y != curY; y -= sign) {
				uint8		v = arp_clip_255(gradCount * 255);
				grad[offset1 + y] = ARP_MAX(grad[offset1 + y], v);
				gradCount += gradStep;
			}
		}
		grad[offset1 + curY] = 255;

		/* Draw lines on either side and bottom image
		 */
		if (alpha == 255) _render_sides(r, g, b, a, w, h, x, y, curY, area, bgC, lineC);

		curV = nextV;

		tmpOffset = offset1;
		offset1 = offset2;
		offset2 = tmpOffset;
	}
	
	delete[] grad;
	return B_OK;
}
