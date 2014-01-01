#include <ArpCore/ArpDebug.h>
#include "GlPublic/GlRect.h"

/***************************************************************************
 * GL-RECT
 ****************************************************************************/
GlRect::GlRect()
		: l(-1), t(-1), r(-1), b(-1)
{
}

GlRect::GlRect(int32 inL, int32 inT, int32 inR, int32 inB)
		: l(inL), t(inT), r(inR), b(inB)
{
}

GlRect::GlRect(const GlRect& o)
		: l(o.l), t(o.t), r(o.r), b(o.b)
{
}

bool GlRect::operator==(const GlRect& o) const
{
	return (l == o.l && t == o.t && r == o.r && b == o.b);
}

bool GlRect::operator!=(const GlRect& o) const
{
	return (l != o.l || t != o.t || r != o.r || b != o.b);
}

status_t GlRect::ConformTo(const GlRect& rect)
{
	if (rect.l < 0 || rect.t < 0) return B_ERROR;
	if (rect.l >= rect.r || rect.t >= rect.b) return B_ERROR;

	if (l < rect.l) l = rect.l;
	else if (l >= rect.r) l = rect.l;
	if (t < rect.t) t = rect.t;
	else if (t >= rect.b) t = rect.t;
	
	if (r < l || r > rect.r) r = rect.r;
	if (b < t || b > rect.b) b = rect.b;

	ArpASSERT(l >= 0 && t >= 0 && r <= rect.r && b <= rect.b && l < r && t < b);
	return B_OK;
}
