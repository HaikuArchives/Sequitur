#include <stdio.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlNodes/GlReplicate.h>

static const int32		GL_REPLICATE_KEY	= 'ApRe';
static const int32		_MAP_KEY			= 'map_';
static const int32		_DEPTH_KEY			= 'dpth';
static const uint32		_ALGO_INDEX			= 0;
static const GlRelAbs	gInit				= GlRelAbs(0.5f, 0);

/***************************************************************************
 * _GL-TILE-FRACTAL-ALGO
 ****************************************************************************/
class _GlReplicateAlgo : public GlAlgo2d
{
public:
	_GlReplicateAlgo(	gl_node_id nid, uint32 targets,
						const GlRelAbs& depth, GlAlgo* algo);
	_GlReplicateAlgo(const _GlReplicateAlgo& o);
	
	virtual GlAlgo*			Clone() const;
	virtual status_t		Process(const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
									GlProcessStatus* status);

private:
	typedef GlAlgo2d		inherited;
	GlRelAbs				mDepth;
	int16					mOffset[256];
		
	status_t				MakeData(uint8* data, int32 w, int32 h, int32 newW, int32 newH);
	void					FillOffsets(int32 index, int32 size, int32 low, int32 high,
										GlCache1d* cache);
	void					Tile(	uint8* src, int32 srcW, int32 srcH,
									int16* dest, int32 destW, int32 destH) const;
};

/***************************************************************************
  * GL-REPLICATE
 ***************************************************************************/
GlReplicate::GlReplicate(const GlReplicateAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this));
}

GlReplicate::GlReplicate(const GlReplicate& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlReplicate::Clone() const
{
	return new GlReplicate(*this);
}

GlAlgo* GlReplicate::Generate(const gl_generate_args& args) const
{
	uint32			targets = PixelTargets();
	GlRelAbs		depth = Params().RelAbs(_DEPTH_KEY, &gInit);
//	float			depth = Params().Float(_DEPTH_KEY);
	GlAlgo*			algo = GenerateChainAlgo(_MAP_KEY, args);

	if (args.flags&GL_NODE_ICON_F) {
		depth.rel = 0.75;
		depth.abs = 0;
		targets = GL_PIXEL_RGBA;
	}
	return new _GlReplicateAlgo(Id(), targets, depth, algo);
}

// #pragma mark -

/***************************************************************************
 * GL-REPLICATE-ADD-ON
 ***************************************************************************/

//	const GlParamType*		mDepth;		// Depends on the mode

GlReplicateAddOn::GlReplicateAddOn()
		: inherited(SZI[SZI_arp], GL_REPLICATE_KEY, SZ(SZ_Combine), SZ(SZ_Replicate), 1, 0)
{
	GlRelAbs		minV(0, 0), maxV(1, 256);
	
//	mDepth =	AddParamType(new GlFloatParamType(_DEPTH_KEY, "Depth", 0, 1, 0.5, 0.01));
	AddParamType(new GlRelAbsParamType(_DEPTH_KEY, SZ(SZ_Depth), minV, maxV, gInit, 0.01f));
}

GlNode* GlReplicateAddOn::NewInstance(const BMessage* config) const
{
	return new GlReplicate(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-REPLICATE-FRACTAL-ALGO
 ****************************************************************************/
_GlReplicateAlgo::_GlReplicateAlgo(	gl_node_id nid, uint32 targets,
									const GlRelAbs& depth, GlAlgo* algo)
		: inherited(nid, targets), mDepth(depth)
{
	ArpVALIDATE(mDepth.abs >= 0, mDepth.abs = 0);
	if (algo) SetChain(algo, _ALGO_INDEX);
}

_GlReplicateAlgo::_GlReplicateAlgo(const _GlReplicateAlgo& o)
		: inherited(o), mDepth(o.mDepth)
{
}

GlAlgo* _GlReplicateAlgo::Clone() const
{
	return new _GlReplicateAlgo(*this);
}

status_t _GlReplicateAlgo::Process(	const GlPlanes* pixels, uint8* data, int32 w, int32 h,
									GlProcessStatus* status)
{
	if (!data) return B_OK;

	/* 0.5 is 2 replications -- anything below that is obviously meaningless.
	 */
	if (mDepth.rel < 0.50 && mDepth.abs < 1) return B_OK;

	/* Figure the total number of replications I can do, then use that
	 * and the depth to figure how many I'm actually going to do.
	 */
	int32		count = 0;
	int32		n = 0;
	int32		div = 2;
	while (w / div > 2 && h / div > 2) {
		count++;
		div += 2;
	}
	if (mDepth.rel >= 0.50)
		n = ARP_ROUND(((mDepth.rel - 0.5) * (count - 1)) / 0.5) + 1;
	n += mDepth.abs;
	if (n < 1) return B_OK;
	else if (n > count) n = count;

	int16*		bigDest = new int16[w * h];
	if (!bigDest) return B_OK;

	int32		pix;
	for (pix = 0; pix < w * h; pix++) bigDest[pix] = data[pix];

	GlAlgo1d*	algo = Algo1dAt(_ALGO_INDEX);
	GlCache1d*	cache = (algo) ? algo->NewCache(n) : 0;
//printf("_GlReplicateAlgo depth %ld\n", n);
	int32		dataW = w, dataH = h;
	div = 2;
	for (int32 k = 0; k < n; k++) {
		int32	newDataW = w / div, newDataH = h / div;
		if (MakeData(data, dataW, dataH, newDataW, newDataH) != B_OK) break;
		dataW = newDataW;
		dataH = newDataH;

		int32	low = 255, high = 0;
		for (int32 pix = 0; pix < dataW * dataH; pix++) {
			if (data[pix] < low) low = data[pix];
			if (data[pix] > high) high = data[pix];
		}
		FillOffsets(k, n, low, high, cache);

		Tile(data, dataW, dataH, bigDest, w, h);
		
		div += 2;
	}

	for (pix = 0; pix < w * h; pix++) data[pix] = arp_clip_255(bigDest[pix]);
	delete[] bigDest;
	delete cache;
	return B_OK;
}

status_t _GlReplicateAlgo::MakeData(uint8* data, int32 w, int32 h, int32 newW, int32 newH)
{
	if (newW >= w || newH >= h) return B_ERROR;
//	ArpASSERT(newW < w && newH < h);
	/* Scale down the x.
	 */
	float			scale = newW / float(w);
	for (int32 y = 0; y < h; y++) {
		float		posX = 0;
		int32		srcX = 0;
		for (int32 destX = 0; destX < newW; destX++) {
			int32	sum = 0, count = 0;
			while (posX < 1) {
				srcX++;
				if (srcX >= w) srcX = w - 1;
				sum += data[ARP_PIXEL(srcX, y, w)];
				count++;
				posX += scale;
			}
			posX -= 1;
			if (count > 1) sum /= count;
			data[ARP_PIXEL(destX, y, newW)] = ARP_CLIP_255(sum);
		}
	}
	/* Scale down the y.
	 */
	scale = newH / float(h);
	for (int32 x = 0; x < newW; x++) {
		float		posY = 0;
		int32		srcY = 0;
		for (int32 destY = 0; destY < newH; destY++) {
			int32	sum = 0, count = 0;
			while (posY < 1) {
				srcY++;
				if (srcY >= h) srcY = h - 1;
				sum += data[ARP_PIXEL(x, srcY, newW)];
				count++;
				posY += scale;
			}
			posY -= 1;
			if (count > 1) sum /= count;
			data[ARP_PIXEL(x, destY, newW)] = ARP_CLIP_255(sum);
		}
	}
	return B_OK;
}

void _GlReplicateAlgo::FillOffsets(	int32 index, int32 size, int32 low, int32 high,
									GlCache1d* cache)
{
	float		mod;
	if (cache) {
		float	v = 0.5; 
		if (size > 1) v = ( (index * 2) / float((size - 1) * 2) );
		v = cache->At(v);
//printf("\tGlTile %ld: mod %f v %f\n", index, mod, v);
		mod = v;
	} else mod = 1 / float((index * 2) + 2);
// Old style (when algo1d's had At() -- new style should work fine
#if 0
	GlAlgo1d*	algo = Algo1dAt(_ALGO_INDEX);
	
	if (algo) {
		float	v = 0.5f; 
		if (size > 1) v = ( (index * 2) / float((size - 1) * 2) );
		v = algo->At(v);
//printf("\tGlTile %ld: mod %f v %f\n", index, mod, v);
		mod = v;
	} else mod = 1 / float((index * 2) + 2);
#endif
	float		half = (high - low) / 2.0f;
	for (int32 k = 0; k < 256; k++) {
		if (k < low || k > high) mOffset[k] = 0;
		else mOffset[k] = int16((k - low - half) * mod);
	}
}

void _GlReplicateAlgo::Tile(uint8* src, int32 srcW, int32 srcH,
							int16* dest, int32 destW, int32 destH) const
{
	int32		srcX, srcY, destX, destY;
	for (destY = 0; destY < destH; destY++) {
		for (destX = 0; destX < destW; destX++) {
			srcX = destX;
			while (srcX >= srcW) srcX -= srcW;
			srcY = destY;
			while (srcY >= srcH) srcY -= srcH;
			ArpASSERT(srcX >= 0 && srcY >= 0);
			
			int32		srcPix = ARP_PIXEL(srcX, srcY, srcW),
						destPix = ARP_PIXEL(destX, destY, destW);

			dest[destPix] += mOffset[src[srcPix]];
		}
	}
}
