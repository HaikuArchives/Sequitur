#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlDegreeLine.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlSupportTypes.h>
#include <GlNodes/GlEarthquakeIm.h>

static const int32		_UNCHANGED			= -1;
static const int32		_EMPTY				= -2;

static const int32		_PLATES_KEY			= 'plts';
static const uint32		_PLATES_INDEX		= 0;
static const int32		_DIR_KEY			= 'dir_';
static const uint32		_DIR_INDEX			= 1;
static const int32		_DIST_KEY			= 'dist';
static const uint32		_DIST_INDEX			= 2;

static const int32		_MO_KEY				= 'morg';
static const bool		_MO_INIT			= false;

class _PlateInfo
{
public:
	int32		l, t, r, b;		// The bounds of the plate in the original image
	int32		deltaX, deltaY;	// The move offset for each pixel.

	_PlateInfo();
	void		Init(	int32 w, int32 h, int32 plate, int32 plateCount,
						GlCache1d* dirCache, GlCache1d* distCache, int32 maxDistance);
	/* Increase the frame to accomodate my delta'd rect.
	 */
	void		GetNewFrame(int32* newL, int32* newT, int32* newR, int32* newB) const;

	void		Move(	GlPlanes& src, int32* plates, GlPlanes& dest,
						int32 plate, int32 offsetX, int32 offsetY);
};

static _PlateInfo*	_get_plate_info(int32* plates, int32 w, int32 h,
									int32 plateCount, GlAlgo1d* dir1d,
									GlAlgo1d* dist1d, int32 maxDistance);
static uint8*		_make_mask(GlImage* img, GlAlgo2d* algo, GlMask& mask);
static void			_init_dest(	GlPlanes& src, int32* plates, GlPlanes& dest,
								ArpVoxel& bg, int32 offsetX, int32 offsetY);

/***************************************************************************
 * _GL-EARTHQUAKE-ALGO
 ***************************************************************************/
class _GlEarthquakeAlgo : public GlAlgoIm
{
public:
	_GlEarthquakeAlgo(	gl_node_id nid, uint32 targets, const GlRelAbs& length,
						bool mo, GlAlgo1d* dir, GlAlgo1d* dist, GlAlgo2d* plates);
	_GlEarthquakeAlgo(const _GlEarthquakeAlgo& o);

	virtual GlAlgo*		Clone() const;

//	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
//	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
	uint32				mTargets;
	GlRelAbs			mLength;
	bool				mMaintainOrigin;

	_FillLine			mStack[_MAX_FILL_LINE];

	status_t			Earthquake(	GlImage* img, uint8* plateData,
									GlAlgo1d* dir1d, GlAlgo1d* dist1d,
									int32 maxDistance);
	status_t			MovePlates(	GlImage* img, int32* plates, int32 plateCount,
									GlAlgo1d* dir1d, GlAlgo1d* dist1d, int32 maxDistance);
	/* Answer the number of plates I created.
	 */
	int32				FillPlates(	int32* plates, uint8* plateData,
									int32 w, int32 h);
};

/***************************************************************************
 * GL-EARTHQUAKE-IM
 ***************************************************************************/
static GlNode* _new_invert_node()
{
	GlNode*		node = GlGlobals().NewNode(GL_INVERT_KEY, 0);
	if (!node) return 0;
	GlChain*	c = 0;
	for (uint32 k = 0; (c = node->ChainAt(k)) != 0; k++) c->DeleteNodes();
	c = node->ChainAt(0);
	if (!c || c->Io() != GL_1D_IO) return node;

	GlNode*		n = GlGlobals().NewNode(GL_STAIR_KEY, 0);
	if (n) c->AddNode(n);

	return node;
}

GlEarthquakeIm::GlEarthquakeIm(	const GlEarthquakeImAddOn* addon,
								const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	SetPixelTargets(GL_PIXEL_ALL);

	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_PLATES_KEY, GL_2D_IO, SZ(SZ_Plates), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_ELLIPSE_KEY, 0);
		if (node) chain->AddNode(node);
		node = _new_invert_node();
		if (node) chain->AddNode(node);
	}

	VerifyChain(new GlChain(_DIR_KEY, GL_1D_IO, SZ(SZ_Direction), this));

	chain = VerifyChain(new GlChain(_DIST_KEY, GL_1D_IO, SZ(SZ_Distance), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_PINK_NOISE_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlEarthquakeIm::GlEarthquakeIm(const GlEarthquakeIm& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlEarthquakeIm::Clone() const
{
	return new GlEarthquakeIm(*this);
}

GlAlgo* GlEarthquakeIm::Generate(const gl_generate_args& args) const
{
	GlAlgo2d*			plate2d = GenerateChain2d(_PLATES_KEY, args);
	if (!plate2d) return 0;

	GlAlgo1d*			dir1d = GenerateChain1d(_DIR_KEY, args);
	GlAlgo1d*			dist1d = GenerateChain1d(_DIST_KEY, args);
	GlRelAbs			length(Params().Float('lgtr'), Params().Int32('lgta'));
	bool				maintainOrigin = Params().Bool(_MO_KEY, _MO_INIT);
	return new _GlEarthquakeAlgo(	Id(), PixelTargets(), length,
									maintainOrigin, dir1d, dist1d, plate2d);
}

status_t GlEarthquakeIm::Process(	GlNodeDataList& list,
									const gl_process_args* args)
{
	gl_generate_args	gargs;
	if (args) gargs.flags = args->flags;
	GlAlgo*				a = Generate(gargs);
	if (!a) return B_NO_MEMORY;
	status_t			err = a->PerformAll(list, args);
	delete a;
	return err;
}

// #pragma mark -

/***************************************************************************
 * GL-EARTHQUAKE-IM-ADD-ON
 ***************************************************************************/
GlEarthquakeImAddOn::GlEarthquakeImAddOn()
		: inherited(SZI[SZI_arp], 'AiEq', SZ(SZ_Distort), SZ(SZ_Earthquake), 1, 0)
{
	AddParamType(new GlFloatParamType('lgtr', SZ(SZ_Rel_length), 0, 1, 0.1f, 0.01f));
	AddParamType(new GlInt32ParamType('lgta', SZ(SZ_Abs_length), -1024, 1024, 0));
	AddParamType(new GlBoolParamType(_MO_KEY, SZ(SZ_Maintain_origin), _MO_INIT));
}

GlNode* GlEarthquakeImAddOn::NewInstance(const BMessage* config) const
{
	return new GlEarthquakeIm(this, config);
}

// #pragma mark -

/***************************************************************************
 * _PLATE-INFO
 ***************************************************************************/
_PlateInfo::_PlateInfo()
{
}

void _PlateInfo::Init(	int32 w, int32 h, int32 plate, int32 plateCount,
						GlCache1d* dirCache, GlCache1d* distCache, int32 maxDistance)
{
	l = w; t = h; r = 0; b = 0;

	float				step = (plate / float(plateCount));
	GlDegreeLine		line( ((dirCache) ? (dirCache->At(step)) : step) * 359);
	float				endX, endY;
	line.GetEnd( ((distCache) ? (distCache->At(step)) : step) * maxDistance, &endX, &endY);
	deltaX = (int32)endX;
	deltaY = (int32)endY;
}

void _PlateInfo::GetNewFrame(int32* newL, int32* newT, int32* newR, int32* newB) const
{
	int32			newV = l + deltaX;
	if (newV < *newL) *newL = newV;
	else if (newV > *newR) *newR = newV;

	newV = t + deltaY;
	if (newV < *newT) *newT = newV;
	else if (newV > *newB) *newB = newV;

	newV = r + deltaX;
	if (newV < *newL) *newL = newV;
	else if (newV > *newR) *newR = newV;

	newV = b + deltaY;
	if (newV < *newT) *newT = newV;
	else if (newV > *newB) *newB = newV;
}

void _PlateInfo::Move(	GlPlanes& src, int32* plates, GlPlanes& dest,
						int32 plate, int32 offsetX, int32 offsetY)
{
	ArpASSERT(src.size == dest.size);
	uint32				k;
	for (int32 y = t; y <= b; y++) {
		for (int32 x = l; x <= r; x++) {
			int32		srcPix = ARP_PIXEL(x, y, src.w);
			if (plates[srcPix] == plate && src.a[srcPix] > 0) {
				ArpASSERT((x + deltaX - offsetX) >= 0);
				ArpASSERT((x + deltaX - offsetX) < dest.w);
				ArpASSERT((y + deltaY - offsetY) >= 0);
				ArpASSERT((y + deltaY - offsetY) < dest.h);
				int32	destPix = ARP_PIXEL(x + deltaX - offsetX, y + deltaY - offsetY, dest.w);
				ArpASSERT(destPix >= 0 && destPix < dest.w * dest.h);
				if (src.a[srcPix] == 255) {
					for (k = 0; k < src.size; k++)
						dest.plane[k][destPix] = src.plane[k][srcPix];
				} else {
					/* Mix the source and dest. 50/50.
					 */
					for (k = 0; k < src.size; k++)
						dest.plane[k][destPix] = arp_clip_255(dest.plane[k][destPix]
											+ ((int16(src.plane[k][srcPix]) - dest.plane[k][destPix]) * 0.5));
				}
//				if (src[srcPix].PixA == 255) dest[destPix] = src[srcPix];
//				else dest[destPix].MixPixelAll(src[srcPix], 0.5);
			}
		}
	}
}

// #pragma mark -

/***************************************************************************
 * _GL-EARTHQUAKE-ALGO
 ***************************************************************************/
_GlEarthquakeAlgo::_GlEarthquakeAlgo(	gl_node_id nid, uint32 targets,
										const GlRelAbs& length,
										bool mo, GlAlgo1d* dir,
										GlAlgo1d* dist, GlAlgo2d* plates)
		: inherited(nid), mTargets(targets), mLength(length),
		  mMaintainOrigin(mo)
{
	if (dir) SetChain(dir, _DIR_INDEX);
	if (dist) SetChain(dist, _DIST_INDEX);
	if (plates) SetChain(plates, _PLATES_INDEX);
}

_GlEarthquakeAlgo::_GlEarthquakeAlgo(const _GlEarthquakeAlgo& o)
		: inherited(o), mTargets(o.mTargets), mLength(o.mLength),
		  mMaintainOrigin(o.mMaintainOrigin)
{
}

GlAlgo* _GlEarthquakeAlgo::Clone() const
{
	return new _GlEarthquakeAlgo(*this);
}

status_t _GlEarthquakeAlgo::Perform(GlNodeDataList& list,
									const gl_process_args* args)
{
	GlAlgo2d*			plate2d = Algo2dAt(_PLATES_INDEX);
	if (!plate2d) return B_OK;
	GlAlgo1d*			dir1d = Algo1dAt(_DIR_INDEX);
	GlAlgo1d*			dist1d = Algo1dAt(_DIST_INDEX);
	
	GlImage*			img;
	GlMask				lengthMask, plateMask;
	for (uint32 k = 0; (img = list.ImageAt(k)) != 0; k++) {
		/* FIX:  A little hack to set the bg to transparent.  What I will
		 * probably do is have a property-setting node once again, and a chain
		 * that returns a background colour.
		 */
		ArpVoxel		bgC = img->Property(ARP_BACKGROUND_COLOUR);
		bgC.a = 0;
		img->SetProperty(ARP_BACKGROUND_COLOUR, bgC);
		 
		uint8*			plateData = _make_mask(img, plate2d, plateMask);
		if (plateData) {
			int32		w = img->Width(), h = img->Height();
			int32		dist = int32(mLength.abs + (((w + h) / 2) * mLength.rel));
			Earthquake(img, plateData, dir1d, dist1d, dist);
		}
	}
	return B_OK;
}
/* A debugging that writes the plate data to the image so I can see it.
 */
#if 0
static void _show_plates_debug(GlImage* img, int32* plates, int32 plateCount)
{
	int32			w, h;
	float			scale = 255.0 / plateCount; 
	GlPlanes*		pixels = img->LockPixels(GL_PIXEL_RGBA);
	if (pixels) {
		for (int32 pix = 0; pix < w * h; pix++) {
			pixels->a[pix] = 255;
			if (plates[pix] < 0) {
				pixels->r[pix] = pixels->b[pix] = 0;
				pixels->g[pix] = 255;
			} else {
				pixels->r[pix] = pixels->g[pix] = pixels->b[pix] = arp_clip_255(plates[pix] * scale);
			}
		}
		img->UnlockPixels(pixels);
	}
}
#endif

status_t _GlEarthquakeAlgo::Earthquake(	GlImage* img, uint8* plateData,
										GlAlgo1d* dir1d, GlAlgo1d* dist1d,
										int32 maxDistance)
{
	ArpVALIDATE(img && plateData, return B_ERROR);
	int32			w = img->Width(), h = img->Height();
	ArpVALIDATE(w > 0 && h > 0, return B_ERROR);
	int32*			plates = new int32[w * h];
	if (!plates) return B_NO_MEMORY;

	int32			plateCount = FillPlates(plates, plateData, w, h);
//	_show_plates_debug(img, plates, plateCount);
	if (plateCount > 0) MovePlates(img, plates, plateCount, dir1d, dist1d, maxDistance);

	delete[] plates;
	return B_OK;
}

status_t _GlEarthquakeAlgo::MovePlates(	GlImage* destImg, int32* plates, int32 plateCount,
										GlAlgo1d* dir1d, GlAlgo1d* dist1d, int32 maxDistance)
{
	int32					srcW = destImg->Width(), srcH = destImg->Height();
	_PlateInfo*				plateInfo = _get_plate_info(plates, srcW, srcH, plateCount,
														dir1d, dist1d, maxDistance);
	if (!plateInfo) return B_NO_MEMORY;
	int32					plate;
#if 0
	printf("Plates (total %ld max distance %ld)\n", plateCount, maxDistance);
	for (plate = 0; plate < plateCount; plate++) {
		printf("Plate %ld: (%ld, %ld) (%ld, %ld) delta (%ld, %ld)\n", plate,
				plateInfo[plate].l, plateInfo[plate].t,
				plateInfo[plate].r, plateInfo[plate].b,
				plateInfo[plate].deltaX, plateInfo[plate].deltaY);
	}
#endif
	int32					newL = 0, newT = 0, newR = srcW - 1, newB = srcH - 1;
	for (plate = 0; plate < plateCount; plate++)
		plateInfo[plate].GetNewFrame(&newL, &newT, &newR, &newB);
	int32					destW = newR - newL + 1, destH = newB - newT + 1;
//	printf("New frame (%ld, %ld) (%ld, %ld) w %ld h %ld\n",
//				newL, newT, newR, newB, destW, destH);
	GlImage*				srcImg = destImg->SourceClone(destW, destH);
	if (srcImg) {
		GlPlanes*			srcPixels = srcImg->LockPixels(mTargets);
		if (srcPixels) {
			GlPlanes*		destPixels = destImg->LockPixels(mTargets);
			if (destPixels) {
#if 0
for (int32 pix = 0; pix < srcPixels->w * srcPixels->h; pix++) {
	for (uint32 k = 0; k < destPixels->size; k++) destPixels->plane[k][pix] = arp_clip_255(plates[pix]);
	destPixels->a[pix] = 255;
}
#endif
#if 1
				ArpASSERT(srcPixels->size == destPixels->size);
				ArpVoxel	bg = destImg->Property(ARP_BACKGROUND_COLOUR);
				_init_dest(*srcPixels, plates, *destPixels, bg, newL, newT);
				
				for (plate = 0; plate < plateCount; plate++)
					plateInfo[plate].Move(	*srcPixels, plates, *destPixels,
											plate, newL, newT);
		
#endif
				destImg->UnlockPixels(destPixels);
			}
			srcImg->UnlockPixels(srcPixels);
		}
		delete srcImg;
	}

	if (mMaintainOrigin && (newL != 0 || newT != 0)) {
		int32			originX, originY;
		destImg->GetOrigin(&originX, &originY);
		destImg->SetOrigin(originX - newL, originY - newT);
	}

	delete[] plateInfo;
	return B_OK;
}

int32 _GlEarthquakeAlgo::FillPlates(int32* plates, uint8* plateData, int32 w, int32 h)
{
	int32			level = 0, pix;
	uint8			map[256];
	memset(map, 0, 256);
	
	for (pix = 0; pix < w * h; pix++) map[plateData[pix]] = 1;

	for (pix = 1; pix < 256; pix++) {
		if (map[pix] > 0) {
			ArpASSERT(level < 256);
			map[pix] = arp_clip_255(level);
			level++;
		}
	}
	ArpASSERT(level < 256);
	
	for (pix = 0; pix < w * h; pix++) {
		if (plateData[pix] == 0) plates[pix] = _UNCHANGED;
		else plates[pix] = map[plateData[pix]];
	}
	return level;
}

// #pragma mark -

/***************************************************************************
 * Misc
 ***************************************************************************/
static _PlateInfo* _get_plate_info(	int32* plates, int32 w, int32 h,
									int32 plateCount, GlAlgo1d* dir1d,
									GlAlgo1d* dist1d, int32 maxDistance)
{
	ArpVALIDATE(plateCount > 0, return 0);
	_PlateInfo*		plateInfo = new _PlateInfo[plateCount];
	if (!plateInfo) return 0;
	int32			x, y, pix;

	GlCache1d*		dirCache = (dir1d) ? dir1d->NewCache(plateCount) : 0;
	GlCache1d*		distCache = (dist1d) ? dist1d->NewCache(plateCount) : 0;
	for (pix = 0; pix < plateCount; pix++)
		plateInfo[pix].Init(w, h, pix, plateCount, dirCache, distCache, maxDistance);
	delete dirCache;
	delete distCache;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			int32	plate = plates[pix];
			ArpASSERT(plate < plateCount);
			if (plate >= 0) {
				if (x < plateInfo[plate].l) plateInfo[plate].l = x;
				if (y < plateInfo[plate].t) plateInfo[plate].t = y;
				if (x > plateInfo[plate].r) plateInfo[plate].r = x;
				if (y > plateInfo[plate].b) plateInfo[plate].b = y;
			}
		}
	}
	
	return plateInfo;
}

static uint8* _make_mask(GlImage* img, GlAlgo2d* algo, GlMask& mask)
{
	ArpVALIDATE(img && algo, return 0);
	uint8*			data = 0;
	GlPlanes*		pixels = img->LockPixels(GL_PIXEL_ALL);
	if (pixels) data = mask.Make(*pixels, algo);
	img->UnlockPixels(pixels);
	return data;
}

static void _init_dest(	GlPlanes& src, int32* plates, GlPlanes& dest,
						ArpVoxel& bg, int32 offsetX, int32 offsetY)
{
	for (int32 y = 0; y < dest.h; y++) {
		for (int32 x = 0; x < dest.w; x++) {
			int32		destPix = ARP_PIXEL(x, y, dest.w);
			int32		srcX = x + offsetX, srcY = y + offsetY;
			int32		srcPix;
			if (srcX >= 0 && srcX < src.w && srcY >= 0 && srcY < src.h
					&& plates[(srcPix = ARP_PIXEL(srcX, srcY, src.w))] == _UNCHANGED) {
				for (uint32 k = 0; k < dest.size; k++) dest.plane[k][destPix] = src.plane[k][srcPix];
			} else {
				/* FIX:  Sorta need an object that encapsulates every
				 * possible component.
				 */
				if (dest.r) dest.r[destPix] = bg.r;
				if (dest.g) dest.g[destPix] = bg.g;
				if (dest.b) dest.b[destPix] = bg.b;
				if (dest.a) dest.a[destPix] = bg.a;
				if (dest.z) dest.z[destPix] = bg.z;
			}
		}
	}
}
