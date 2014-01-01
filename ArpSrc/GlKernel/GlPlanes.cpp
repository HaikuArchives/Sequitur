#include <math.h>
#include "GlPublic/GlDefs.h"
#include "GlPublic/GlPixel.h"
#include "GlPublic/GlPlanes.h"

static inline float gl_min_3(float a1, float a2, float a3);
static inline float gl_max_3(float a1, float a2, float a3);

static uint8			gScaledR[256], gScaledG[256], gScaledB[256];
static bool				gInitValue = false;

/***************************************************************************
 * GL-PLANES
 ****************************************************************************/
GlPlanes::GlPlanes(int32 inW, int32 inH)
		: w(inW), h(inH), size(0), plane(0),
		  planeSrc(0), r(0), g(0), b(0), a(0), z(0), diff(0), spec(0), d(0), c(0), f(0),
		  mOwner(false)
{
}

GlPlanes::GlPlanes(int32 inW, int32 inH, uint32 s)
		: w(inW), h(inH), size(0), plane(0),
		  planeSrc(0), r(0), g(0), b(0), a(0), z(0), diff(0), spec(0), d(0), c(0), f(0),
		  mOwner(true)
{
	if (w < 1 || h < 1 || s < 1) return;
	plane = new uint8*[s];
	if (!plane) return;
	size = s;
	uint32		k;
	for (k = 0; k < size; k++) plane[k] = new uint8[w * h];
	/* Make sure everyone alloc'ed, fail otherwise.
	 */
	for (k = 0; k < size; k++) {
		if (!plane[k]) {
			Free();
			return;
		}
	}
}

static uint8* _replace_plane(uint8** oldPlane, uint8* src, uint8** newPlane, uint32 size)
{
	for (uint32 k = 0; k < size; k++) {
		if (oldPlane[k] == src) return newPlane[k];
	}
	return 0;
}

GlPlanes::GlPlanes(const GlPlanes& o)
		: w(0), h(0), size(0), plane(0),
		  planeSrc(0), r(0), g(0), b(0), a(0), z(0), diff(0), spec(0), d(0), c(0), f(0),
		  mOwner(true)
{
	if (o.w < 1 || o.h < 1 || o.size < 1) return;
	plane = new uint8*[o.size];
	if (!plane) return;
	size = o.size;
	w = o.w;
	h = o.h;
	uint32		k;
	for (k = 0; k < size; k++) plane[k] = new uint8[w * h];
	/* Make sure everyone alloc'ed, fail otherwise.
	 */
	for (k = 0; k < size; k++) {
		if (!plane[k]) {
			Free();
			return;
		}
	}
	if (o.planeSrc && (planeSrc = new uint32[o.size]) == 0) {
		Free();
		return;
	}
	/* Copy the data.
	 */
	for (k = 0; k < size; k++) memcpy(plane[k], o.plane[k], w * h);
	/* Make sure any targets point to the myself.
	 */
	if (o.r) r			= _replace_plane(o.plane, o.r, plane, size);
	if (o.g) g			= _replace_plane(o.plane, o.g, plane, size);
	if (o.b) b			= _replace_plane(o.plane, o.b, plane, size);
	if (o.a) a			= _replace_plane(o.plane, o.a, plane, size);
	if (o.z) z			= _replace_plane(o.plane, o.z, plane, size);
	if (o.diff) diff	= _replace_plane(o.plane, o.diff, plane, size);
	if (o.spec) spec	= _replace_plane(o.plane, o.spec, plane, size);
	if (o.d) d			= _replace_plane(o.plane, o.d, plane, size);
	if (o.c) c			= _replace_plane(o.plane, o.c, plane, size);
	if (o.f) f			= _replace_plane(o.plane, o.f, plane, size);
}

GlPlanes::~GlPlanes()
{
	/* There should never be data in here when I'm deleted, since that
	 * should always be wiped when I'm unlocked.  And if it isn't wiped,
	 * that means someone apparently isn't unlocking me.
	 */
// Oops, that was true for the GlPixels subclass, not true for the planes.
//	ArpASSERT(size == 0);
	Free();
}

uint8 GlPlanes::RgbValue(uint8 r, uint8 g, uint8 b)
{
	ArpASSERT(gInitValue);
	if (gInitValue) return arp_clip_255(gScaledR[r] + gScaledG[g] + gScaledB[b]);
	uint8		v = (r > g) ? r : g;
	return (v > b) ? v : b;
}

GlPlanes* GlPlanes::Clone() const
{
	return new GlPlanes(*this);
}

void GlPlanes::Free()
{
	if (mOwner) {
		for (uint32 k = 0; k < size; k++) delete[] plane[k];
	}
	w = h = 0;
	size = 0;
	delete[] plane;
	plane = 0;
	delete[] planeSrc;
	planeSrc = 0;
	r = g = b = a = z = diff = spec = d = c = f = 0;
}

status_t GlPlanes::SetSize(uint32 planeSize)
{
	int32	oldW = w, oldH = h;
	Free();
	if (planeSize < 1) return B_OK;
	plane = new uint8*[planeSize];
	if (!plane) return B_NO_MEMORY;
	size = planeSize;
	for (uint32 k = 0; k < size; k++) plane[k] = 0;
	w = oldW;
	h = oldH;
	return B_OK;	
}

void GlPlanes::GetHsv(int32 pix, float* hue, float* s, float* v) const
{
	ArpASSERT(HasColor());
	ArpASSERT(pix >= 0 && pix < w * h);
	float		fr = glTable256[r[pix]];
	float		fg = glTable256[g[pix]];
	float		fb = glTable256[b[pix]];
	float		min, max;
	min = gl_min_3(fr, fg, fb);
	max = gl_max_3(fr, fg, fb);
	*v = max;							// v
	if (!hue || !s) return;
	float		delta = max - min;
	if (max != 0)
		*s = delta / max;				// s
	else {
		// r = g = b = 0				// s = 0, hue is undefined
		*s = 0;
		*hue = -1;
		return;
	}
	if (fr == max)
		*hue = (fg - fb) / delta;			// between yellow & magenta
	else if (fg == max)
		*hue = 2 + (fb - fr) / delta;		// between cyan & yellow
	else
		*hue = 4 + (fr - fg) / delta;		// between magenta & cyan
	*hue *= 60;							// degrees
	if (*hue < 0)
		*hue += 360;
}

void GlPlanes::SetHsv(int32 pix, float hue, float s, float v)
{
	ArpASSERT(HasColor());
	ArpASSERT(pix >= 0 && pix < w * h);
	int		i;
	float	f, p, q, t;
	if (s == 0) {
		// achromatic (grey)
		r[pix] = g[pix] = b[pix] = arp_clip_255(v * 255);
		return;
	}
	hue /= 60;                        // sector 0 to 5
	i = int(floor(hue));
	f = hue - i;                      // factorial part of hue
	p = v * (1 - s);
	q = v * (1 - s * f);
	t = v * (1 - s * (1 - f));
	switch (i) {
		case 0:
			r[pix] = arp_clip_255(v * 255);
			g[pix] = arp_clip_255(t * 255);
			b[pix] = arp_clip_255(p * 255);
			break;
		case 1:
			r[pix] = arp_clip_255(q * 255);
			g[pix] = arp_clip_255(v * 255);
			b[pix] = arp_clip_255(p * 255);
			break;
		case 2:
			r[pix] = arp_clip_255(p * 255);
			g[pix] = arp_clip_255(v * 255);
			b[pix] = arp_clip_255(t * 255);
			break;
		case 3:
			r[pix] = arp_clip_255(p * 255);
			g[pix] = arp_clip_255(q * 255);
			b[pix] = arp_clip_255(v * 255);
			break;
		case 4:
			r[pix] = arp_clip_255(t * 255);
			g[pix] = arp_clip_255(p * 255);
			b[pix] = arp_clip_255(v * 255);
			break;
		default:
			r[pix] = arp_clip_255(v * 255);
			g[pix] = arp_clip_255(p * 255);
			b[pix] = arp_clip_255(q * 255);
			break;
	}
}

uint8 GlPlanes::Hue(int32 pix) const
{
	float		h, s, v;
	GetHsv(pix, &h, &s, &v);
	if (h < 0) return 0;
	return arp_clip_255((h / 360) * 255);
}

uint8 GlPlanes::Saturation(int32 pix) const
{
	float		h, s, v;
	GetHsv(pix, &h, &s, &v);
	return arp_clip_255(s * 255);
}

uint8 GlPlanes::Value(int32 pix) const
{
	ArpASSERT(HasColor());
	ArpASSERT(pix >= 0 && pix < w * h);
	if (gInitValue)
		return arp_clip_255(gScaledR[r[pix]] + gScaledG[g[pix]] + gScaledB[b[pix]]);
	uint8		v = (r[pix] > g[pix]) ? r[pix] : g[pix];
	return (v > b[pix]) ? v : b[pix];
}

bool GlPlanes::HasColor() const
{
	return HasPlanes(GL_PIXEL_R_MASK | GL_PIXEL_G_MASK | GL_PIXEL_B_MASK | GL_PIXEL_A_MASK | GL_PIXEL_Z_MASK);
}

bool GlPlanes::HasPlanes(uint32 mask) const
{
	if (mask&GL_PIXEL_R_MASK && !r) return false;
	if (mask&GL_PIXEL_G_MASK && !g) return false;
	if (mask&GL_PIXEL_B_MASK && !b) return false;
	if (mask&GL_PIXEL_A_MASK && !a) return false;
	if (mask&GL_PIXEL_Z_MASK && !z) return false;
	if (mask&GL_PIXEL_DIFF_MASK && !diff) return false;
	if (mask&GL_PIXEL_SPEC_MASK && !spec) return false;
	if (mask&GL_PIXEL_D_MASK && !d) return false;
	if (mask&GL_PIXEL_C_MASK && !c) return false;
	if (mask&GL_PIXEL_F_MASK && !f) return false;
	return true;
}

bool GlPlanes::IncFrame(uint32 total)
{
	return false;
}

void GlPlanes::Rewind()
{
}

void GlPlanes::Fill(GlFillType ft)
{
	if (w < 1 || h < 1) return;
	if (ft == GL_FILL_BLACK) Black();
	else if (ft == GL_FILL_COLORWHEEL) ColorWheel(0, 0, w-1, h-1);
}

void GlPlanes::ColorWheel(int32 l, int32 t, int32 r, int32 b)
{
	ArpVALIDATE(a, return);
	int32				wheelHalfW = (r - l) / 2, wheelHalfH = (b - t) / 2;
	int32				cenX = l + wheelHalfW, cenY = t + wheelHalfH;
	float				radius = float((wheelHalfW < wheelHalfH) ? wheelHalfW : wheelHalfH);
	float				radiusHalf = radius / 2;
	
	if (t < 0) t = 0;
	if (b >= h) b = h -1;
	if (l < 0) l = 0;
	if (r >= w) r = w - 1;
	
	for (int32 y = t; y <= b; y++) {
		for (int32 x = l; x <= r; x++) {
			int32		pix = ARP_PIXEL(x, y, w);
			float		d = ARP_DISTANCE(cenX, cenY, x, y);
			if (d <= radius) {
				float	hue = arp_degree(float(x - cenX), float(cenY - y));
				float	saturation = 1, value = 1;
				if (d < radiusHalf) saturation = 1 - ((radiusHalf - d) / radiusHalf);
				else if (d > radiusHalf) value = 1 - ((d - radiusHalf) / radiusHalf);
				SetHsv(pix, hue, saturation, value);
//				SetHsv(pix, hue, 1, 1);
				a[pix] = 255;
			} else a[pix] = 0;
		}
	}
}

void GlPlanes::Black()
{
	ArpVALIDATE(r && g && b && a, return);
	int32		size = w * h;
	ArpVALIDATE(size > 0, return);
	memset(r,	0,		size);
	memset(g,	0,		size);
	memset(b,	0,		size);
	memset(a,	255,	size);
}

// #pragma mark -

/*******************************************************
 * Miscellaneous functions
 *******************************************************/
void gl_pixel_init_value()
{
	for (int32 k = 0; k < 256; k++) {
		gScaledR[k] = arp_clip_255(0.299 * k);
		gScaledG[k] = arp_clip_255(0.587 * k);
		gScaledB[k] = arp_clip_255(0.114 * k);
	}
	gInitValue = true;
}

static inline float gl_min_3(float a1, float a2, float a3)
{
	float		m = (a1 < a2) ? a1 : a2;
	m = (m < a3) ? m : a3;
	return m;
}

static inline float gl_max_3(float a1, float a2, float a3)
{
	float		m = (a1 > a2) ? a1 : a2;
	m = (m > a3) ? m : a3;
	return m;
}
