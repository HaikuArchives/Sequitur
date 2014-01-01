#include <be/StorageKit.h>
#include <be/TranslationKit.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpPainter.h>

/*******************************************************
 * ARP-BITMAP
 *******************************************************/
ArpBitmap::ArpBitmap()
		: mBitmap(0)
{
}

ArpBitmap::ArpBitmap(float width, float height)
		: mBitmap(0)
{
	mBitmap = new BBitmap(BRect(0, 0, width, height), B_RGBA32);
	if (mBitmap) mPa.set_to(mBitmap->ColorSpace());
}

ArpBitmap::ArpBitmap(const ArpBitmap& o)
		: mBitmap(0)
{
	if (o.mBitmap) mBitmap = new BBitmap(o.mBitmap);
	if (mBitmap) mPa.set_to(mBitmap->ColorSpace());
}

ArpBitmap::ArpBitmap(const BBitmap* o)
		: mBitmap(0)
{
	if (o) mBitmap = new BBitmap(o);
	if (mBitmap) mPa.set_to(mBitmap->ColorSpace());
}

ArpBitmap::ArpBitmap(const BString16& filename)
		: mBitmap(0)
{
	mBitmap = BTranslationUtils::GetBitmap(&filename);
	/* All files need to be in B_RGBA_32
	 */
	ConformColorSpace();
	if (mBitmap) mPa.set_to(mBitmap->ColorSpace());
}

ArpBitmap::ArpBitmap(uint8* r, uint8* g, uint8* b, uint8* a, int32 w, int32 h)
		: mBitmap(0)
{
	Set(r, g, b, a, w, h);
}

ArpBitmap::ArpBitmap(uint8 r, uint8 g, uint8 b, uint8 a, int32 w, int32 h)
		: mBitmap(0)
{
	Set(r, g, b, a, w, h);
}

ArpBitmap::~ArpBitmap()
{
	delete mBitmap;
	mBitmap = 0;
}

int32 ArpBitmap::Width() const
{
	return int32(Bounds().Width());
}

int32 ArpBitmap::Height() const
{
	return int32(Bounds().Height());
}

BRect ArpBitmap::Bounds() const
{
	if (!mBitmap) return BRect();
// Ugh -- be bitmaps report bounds as actual ending pixels.  That
// is, a bitmap that's one pixel wide -- i.e. from 0 to 0 -- is
// reported as 0 to 0.  Hope this doesn't mess things up.
	BRect		b(mBitmap->Bounds());
	b.right++;
	b.bottom++;
	return b;
//	return BRect(mBitmap->Bounds());
}

#if 0
	status_t			GetPixel(	int32 x, int32 y, ArpColour* c,
									ArpPixelRules rules = ARP_NO_PIXEL_RULES) const;
	status_t			SetPixel(int32 x, int32 y, const ArpColour& c);

status_t ArpBitmap::GetPixel(int32 x, int32 y, ArpColour* c, ArpPixelRules rules) const
{
	ArpVALIDATE(mBitmap && c, return B_ERROR);

	if (rules != ARP_NO_PIXEL_RULES) {
		BRect		b = Bounds();
		arp_apply_pixel_rules(&x, &y, int32(b.W()), int32(b.H()), rules);
	}
	ArpASSERT(x >= 0 && x < Bounds().W() && y >= 0 && y < Bounds().H());

	uint8*		pixel = (uint8*)( ((uint8*)mBitmap->Bits()) + (uint32)(x * mPa.bpp() )
						+ (uint32)(y * mBitmap->BytesPerRow() ) );
	rgb_color	tc = mPa.read(pixel);
	c->Set(float(tc.red) / 255, float(tc.green) / 255, float(tc.blue) / 255, float(tc.alpha) / 255);
	return B_OK;
}

status_t ArpBitmap::SetPixel(int32 x, int32 y, const ArpColour& c)
{
	ArpVALIDATE(mBitmap, return B_ERROR);

	uint8*		pixel = (uint8*)( ((uint8*)mBitmap->Bits()) + (uint32)(x * mPa.bpp() )
						+ (uint32)(y * mBitmap->BytesPerRow() ) );
	mPa.write(pixel, c.mC);
	return B_OK;
}
#endif

status_t ArpBitmap::Set(uint8* r, uint8* g, uint8* b, uint8* a,
						int32 w, int32 h)
{
	ArpVALIDATE(r && g && b && a && w > 0 && h > 0, return B_ERROR);
	BBitmap*		bm = new BBitmap(BRect(0, 0, float(w - 1), float(h - 1)), B_RGBA32);
	if (!bm || bm->InitCheck() != B_OK) {
		delete bm;
		return B_NO_MEMORY;
	}
	bm->Set(r, g, b, a, w, h);
/*	
	uint8*			data = (uint8*)bm->Bits();
	int32			dest = 0;
	for (int32 src = 0; src < w * h; src++) {
		data[dest++] = b[src];
		data[dest++] = g[src];
		data[dest++] = r[src];
		data[dest++] = a[src];
	}
*/
	return TakeBitmap(bm);
}

status_t ArpBitmap::Set(uint8 r, uint8 g, uint8 b, uint8 a,
						int32 w, int32 h)
{
	ArpVALIDATE(w > 0 && h > 0, return B_ERROR);
	BBitmap*		bm = new BBitmap(BRect(0, 0, float(w - 1), float(h - 1)), B_RGBA32);
	if (!bm || bm->InitCheck() != B_OK) {
		delete bm;
		return B_NO_MEMORY;
	}

	bm->Fill(r, g, b, a, w, h);	
/*
	uint8*			data = (uint8*)bm->Bits();
	int32			dest = 0;
	for (int32 src = 0; src < w * h; src++) {
		data[dest++] = b;
		data[dest++] = g;
		data[dest++] = r;
		data[dest++] = a;
	}
*/
	return TakeBitmap(bm);
}

status_t ArpBitmap::Get(uint8* r, uint8* g, uint8* b, uint8* a,
						int32 inW, int32 inH) const
{
	if (!mBitmap) return B_ERROR;
	ArpVALIDATE(r && g && b && a, return B_ERROR);

	return mBitmap->Get(r, g, b, a, inW, inH);
}

#if 0
static void _os_pixels_from_RGBA32(	uint8* r, uint8* g, uint8* b, uint8* a,
									int32 w, int32 h, uint8* data)
{
	for (int32 pix = 0; pix < w * h; pix++) {
		int32		byte = pix * 4;
		b[pix] = data[byte];
		g[pix] = data[byte + 1];
		r[pix] = data[byte + 2];
		a[pix] = data[byte + 3];
	}
}

status_t ArpBitmap::Get(uint8* r, uint8* g, uint8* b, uint8* a,
						int32 inW, int32 inH) const
{
	if (!mBitmap) return B_ERROR;
	ArpVALIDATE(r && g && b && a, return B_ERROR);
	/* FIX: Currently, the ArpBitmap guarantees all bitmaps are B_RGBA32.  This
	 * isn't desireable, though, what I need to do is NOT force that conversion
	 * in the ArpBitmap, then in here, handle the easier / more common colour
	 * formats, then for any I don't handle force the conversion the slow way
	 * (i.e. reading each pixel via the get_pixel call).
	 */
	ArpVALIDATE(mBitmap->ColorSpace() == B_RGBA32 || mBitmap->ColorSpace() == B_RGB32, return B_ERROR);

	int32			w = int32(mBitmap->Bounds().Width()) + 1;
	int32			h = int32(mBitmap->Bounds().Height()) + 1;
	ArpVALIDATE(w == inW && h == inH, return B_ERROR);
	uint8*			data = (uint8*)mBitmap->Bits();
	if (w < 1 || h < 1 || !data) return B_ERROR;
	
	if (mBitmap->ColorSpace() == B_RGBA32) _os_pixels_from_RGBA32(r, g, b, a, w, h, data);
	else if (mBitmap->ColorSpace() == B_RGB32) _os_pixels_from_RGBA32(r, g, b, a, w, h, data);
	else ArpASSERT(false);

	return B_OK;
}
#endif

static void os_pixels_from_text_RGBA32( uint8* r, uint8* g, uint8* b, uint8* a, uint8* z,
										int32 w, int32 h, uint8* data,
										const ArpVoxel& bg, const ArpVoxel& fg)
{
	int32			byte;
	float			scale;
	for (int32 pix = 0; pix < w * h; pix++) {
		byte = pix * 4;
		if (data[byte] == 0) {
			r[pix] = bg.r;
			g[pix] = bg.g;
			b[pix] = bg.b;
			a[pix] = bg.a;
			z[pix] = bg.z;
//			pixels[pix].PixDiff = 0;
		} else if (data[byte] == 255) {
			r[pix] = fg.r;
			g[pix] = fg.g;
			b[pix] = fg.b;
			a[pix] = fg.a;
			z[pix] = fg.z;
//			pixels[pix].PixDiff = 255;
		} else {
			scale = float(data[byte]) / 255;
			b[pix]	= arp_clip_255(bg.b + ((fg.b - bg.b) * scale));
			g[pix]	= arp_clip_255(bg.g + ((fg.g - bg.g) * scale));
			r[pix]	= arp_clip_255(bg.r + ((fg.r - bg.r) * scale));
			a[pix]	= arp_clip_255(bg.a + ((fg.a - bg.a) * scale));
			z[pix]	= arp_clip_255(bg.z + ((float(fg.z) - bg.z) * scale));
#if 0
/* Diffusion is a special case for now -- make the back have no diffusion and the
 * front be completely diffuse.
 */
//			pixels[pix].PixDiff = arp_clip_255(scale * 100);
			pixels[pix].PixDiff = 255;
//			pixels[pix].PixDiff = bg.PixDiff;
			pixels[pix].PixSpec = bg.PixSpec;
			pixels[pix].PixD = bg.PixD;
			pixels[pix].PixC = bg.PixC;
			pixels[pix].PixF = bg.PixF;
#endif
		}
	}
}

status_t ArpBitmap::GetTextHack(uint8* r, uint8* g, uint8* b, uint8* a, uint8* z,
								int32 inW, int32 inH,
								const ArpVoxel& bg, const ArpVoxel& fg) const
{
ArpPOLISH("Guess I have to move to BBitmap");
return B_ERROR;
#if 0
	if (!mBitmap) return B_ERROR;
	ArpVALIDATE(r && g && b && a && z, return B_ERROR);
	ArpVALIDATE(mBitmap->ColorSpace() == B_RGBA32 || mBitmap->ColorSpace() == B_RGB32, return B_ERROR);
	int32			w = Width(), h = Height();
	ArpVALIDATE(w == inW && h == inH, return B_ERROR);
	uint8*			data = (uint8*)mBitmap->Bits();
	if (w < 1 || h < 1 || !data) return B_ERROR;
	
	if (mBitmap->ColorSpace() == B_RGBA32)
		os_pixels_from_text_RGBA32(r, g, b, a, z, w, h, data, bg, fg);
	else if (mBitmap->ColorSpace() == B_RGB32)
		os_pixels_from_text_RGBA32(r, g, b, a, z, w, h, data, bg, fg);
	else ArpASSERT(false);

	return B_OK;	
#endif
}

static status_t be_format_from_arp_format(int32 arpFormat, uint32* outBeFormat)
{
	if (arpFormat == ARP_BMP_FORMAT) {
		*outBeFormat = B_BMP_FORMAT;
		return B_OK;
	} else if (arpFormat == ARP_PNG_FORMAT) {
		*outBeFormat = B_PNG_FORMAT;
		return B_OK;
	} else if (arpFormat == ARP_JPG_FORMAT) {
		*outBeFormat = B_JPEG_FORMAT;
		return B_OK;
	} else if (arpFormat == ARP_TGA_FORMAT) {
		*outBeFormat = B_TGA_FORMAT;
		return B_OK;
	}
	return B_ERROR;
}

status_t ArpBitmap::Save(const BString16& filename, int32 format)
{
	if (!mBitmap) return B_ERROR;
	if (filename.Length() < 1) return B_ERROR;
	uint32				beFormat;
	status_t			err = be_format_from_arp_format(format, &beFormat);
	if (err != B_OK) return err;
	BTranslatorRoster*	roster = BTranslatorRoster::Default();
	if (!roster) return B_ERROR;
	BFile				file(&filename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ((err = file.InitCheck()) != B_OK) return err;
	BBitmap*			bm = new BBitmap(mBitmap);
	if (!bm) return B_NO_MEMORY;
	BBitmapStream		stream(bm);
	return roster->Translate(&stream, NULL, NULL, &file, beFormat);
}

status_t ArpBitmap::TrimFromTextHack()
{
ArpPOLISH("Guess I have to move to BBitmap");
return B_ERROR;
#if 0
	if (!mBitmap) return B_ERROR;

	if (!(mBitmap->ColorSpace() == B_RGBA32 || mBitmap->ColorSpace() == B_RGB32)) return B_ERROR;
	int32			w = int32(mBitmap->Bounds().Width()) + 1;
	int32			h = int32(mBitmap->Bounds().Height()) + 1;
	uint8*			data = (uint8*)mBitmap->Bits();
	if (w < 1 || h < 1 || !data) return B_OK;
	
	int32			l = w, t = h, r = 0, b = 0, x, y;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			int32	pix = y * w + x;
			int32	byte = pix * 4;
			if (data[byte] > 0) {
				if (x < l) l = x;
				if (y < t) t = y;
				if (x > r) r = x;
				if (y > b) b = y;
			}
		}
	}
	if (l == 0 && t == 0 && r == w - 1 && b == h - 1) return B_OK;
	
	int32			newW = r - l + 1, newH = b - t + 1;
	BBitmap*		newBm = new BBitmap(BRect(0, 0, float(newW - 1), float(newH - 1)), mBitmap->ColorSpace());
	if (!newBm) return B_NO_MEMORY;
	uint8*			newData = (uint8*)newBm->Bits();

	int32			destByte = 0;
	for (y = 0; y < newH; y++) {
		for (x = 0; x < newW; x++) {
			int32	srcByte = ((y + t) * w + (x + l)) * 4;
			newData[destByte] = data[srcByte];
			newData[destByte + 1] = data[srcByte + 1];
			newData[destByte + 2] = data[srcByte + 2];
			newData[destByte + 3] = data[srcByte + 3];
			destByte += 4;
		}
	}
	
	delete mBitmap;
	mBitmap = newBm;
	if (mBitmap) mPa.set_to(mBitmap->ColorSpace());
	return B_OK;
#endif
}

status_t ArpBitmap::ConformColorSpace()
{
	if (!mBitmap) return B_NO_INIT;
	color_space			cs = mBitmap->ColorSpace();
	if (cs == B_RGBA32) return B_OK;
	if (cs == B_RGB32) return B_OK;
//printf("ColorSpace is %d\n", mBitmap->ColorSpace());
	BBitmap*		bm = mBitmap->AsColorSpace(B_RGBA32);
	if (!bm) return B_NO_MEMORY;
	delete mBitmap;
	mBitmap = bm;
	return B_OK;

#if 0
	BBitmap*		bm = mBitmap;
	mBitmap = new BBitmap(bm->Bounds(), B_RGBA32);
	if (!mBitmap) mBitmap = bm;
	else {
		ArpPainter	painter(this);
		painter.DrawBitmap(bm, BPoint(0, 0));
		delete bm;
	}
#endif
}

const BBitmap* ArpBitmap::Bitmap() const
{
	return mBitmap;
}

status_t ArpBitmap::TakeBitmap(BBitmap* bm)
{
	ArpASSERT(bm != mBitmap);
	delete mBitmap;
	mBitmap = bm;
	if (mBitmap) mPa.set_to(mBitmap->ColorSpace());
	return B_OK;
}

void ArpBitmap::Print(uint32 tabs) const
{
	printf("ArpBitmap w %ld h %ld color space %ld\n", Width(), Height(),
			(mBitmap) ? int32(mBitmap->ColorSpace()) : 0);
}
