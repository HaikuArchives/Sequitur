#include <math.h>
#include <be/support/Errors.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlDegreeLine.h>
#include <GlPublic/GlLineCache.h>

/***************************************************************************
 * GL-DEGREE-LINE
 ****************************************************************************/
GlDegreeLine::GlDegreeLine(	float angle, float x, float y,
							int8 xPos, int8 yPos)
{
	Set(angle, x, y, xPos, yPos);
}

GlDegreeLine::GlDegreeLine(const GlDegreeLine& o)
		: mSlope(o.mSlope), mX(o.mX), mY(o.mY), mCurX(o.mCurX),
		  mCurY(o.mCurY), mCounter(o.mCounter), mSignX(o.mSignX),
		  mSignY(o.mSignY)
{
}

GlDegreeLine& GlDegreeLine::operator=(const GlDegreeLine& o)
{
	mSlope = o.mSlope;
	mX = o.mX;
	mY = o.mY;
	mCurX = o.mCurX;
	mCurY = o.mCurY;
	mCounter = o.mCounter;
	mSignX = o.mSignX;
	mSignY = o.mSignY;
	return *this;
}

void GlDegreeLine::SetAngle(float angle)
{
	SlopeFromDegree(angle);
	if (angle <= 90 || angle >= 270) mSignX = 1;
	else mSignX = -1;
	if (angle >= 180) mSignY = 1;
	else mSignY = -1;
}

void GlDegreeLine::SetOrigin(float x, float y)
{
	mCurX = mX = x;
	mCurY = mY = y;
	mCounter = 0;
}

void GlDegreeLine::SetSigns(int8 xPos, int8 yPos)
{
	if (xPos < 0) mSignX = -1;
	else if (xPos > 0) mSignX = 1;
	if (yPos < 0) mSignY = -1;
	else if (yPos > 0) mSignY = 1;
}

void GlDegreeLine::Set(float angle, float x, float y, int8 xPos, int8 yPos)
{
	SetAngle(angle);
	SetOrigin(x, y);
	SetSigns(xPos, yPos);
}

void GlDegreeLine::GetNext(int32* outX, int32* outY)
{
	if (mSlope <= 0) {
		mCurX += mSignX;
	} else if (mSlope < 1) {
		mCurX += mSignX;
		mCounter += mSlope;
		if (mCounter >= 1) {
			mCurY += mSignY;
			mCounter = mCounter - 1;
		}
	} else if (mSlope == 1) {
		mCurX += mSignX;
		mCurY += mSignY;
	} else {
		mCurY += mSignY;
		mCounter += 1;
		if (mCounter >= mSlope) {
			mCurX += mSignX;
			mCounter = mCounter - mSlope;
		}
	}

	*outX = int32(mCurX);
	*outY = int32(mCurY);
}

status_t GlDegreeLine::CacheLine(GlLineCache& cache)
{
	int32		x, y;
	for (uint32 k = 0; k < cache.size; k++) {
		GetNext(&x, &y);
		cache.x[k] = x;
		cache.y[k] = y;
	}
	return B_OK;
}

void GlDegreeLine::PeekNext(int32* outX, int32* outY)
{
	float		curX = mCurX, curY = mCurY;
	if (mSlope <= 0) {
		curX += mSignX;
	} else if (mSlope < 1) {
		curX += mSignX;
		if ((mCounter + mSlope) >= 1)
			curY += mSignY;
	} else if (mSlope == 1) {
		curX += mSignX;
		curY += mSignY;
	} else {
		curY += mSignY;
		if ((mCounter + 1) >= mSlope)
			curX += mSignX;
	}

	*outX = int32(curX);
	*outY = int32(curY);
}

void GlDegreeLine::GetEnd(float radius, float* outX, float* outY)
{
	arp_new_pt_on_line(mCurX, mCurY, mSlope, radius, outX, outY);
	if (mSignX < 0) *outX = (mCurX - (*outX - mCurX));
	if (mSignY < 0) *outY = (mCurY - (*outY - mCurY));
}

void GlDegreeLine::GetSlope(float* slope, int8* signX, int8* signY) const
{
	*slope = mSlope;
	*signX = mSignX;
	*signY = mSignY;
}

void GlDegreeLine::SlopeFromDegree(float degree)
{
	mSignX = mSignY = 1;
	/* I only figure out slope for 1/4 of the 360 circle, so
	 * render the degrees down into the proper quarter.
	 */
	if (degree < 0) degree = 0;
	else if (degree <= 90) {
		mSignY = -1;
	} else if (degree <= 180) {
		degree = 180 - degree;
		mSignX = mSignY = -1;
	} else if (degree <= 270) {
		degree = degree - 180;
		mSignX = -1;
		mSignY = 1;
	}
	else if (degree <= 359) {
		degree = 360 - degree;
	} else degree = 0;
	/* Calculate the slope.
	 */
	ArpASSERT(degree >= 0 && degree <= 90);
	if (degree == 0) mSlope = 0;
	else if (degree == 45) mSlope = 1;
	else {
		float		radians = float(degree * ARP_DTOR);
		float		tangent = tan(radians);
		mSlope = tangent;
	}
}

// OBSOLETE --
GlDegreeLine::GlDegreeLine(float degree, bool xPositive, bool yPositive)
{
	SlopeFromDegree(degree);
	SetOrigin(0, 0);
	SetXPositive(xPositive);
	SetYPositive(yPositive);
}

void GlDegreeLine::SetXPositive(bool pos)
{
	if (pos) mSignX = 1;
	else mSignX = -1;
}

void GlDegreeLine::SetYPositive(bool pos)
{
	if (pos) mSignY = 1;
	else mSignY = -1;
}

void GlDegreeLine::SetAll(float angle)
{
	SetAngle(angle);
}


