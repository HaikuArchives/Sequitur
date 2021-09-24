/*
	ArpLayoutTools.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	Miscellaneous things used by the ArpLayout library.
*/

#ifndef ARPLAYOUT_ARPDIMENS_H
#include <ArpLayout/ArpDimens.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <float.h>
//#include <algobase.h>

#include <support/Autolock.h>

ArpMOD();

const float ArpAnySize = FLT_MAX/10;

bool IsAnySize(float val)
{
	return val >= ArpAnySize ? true : false;
}

// --------------------------------------------------------------------------
//                               ArpUniDimens
// --------------------------------------------------------------------------

ArpUniDimens::ArpUniDimens()
{
	Init();
}

ArpUniDimens::ArpUniDimens(float pre, float minb, float prefb, float maxb, float post)
{
	SetTo(pre, minb, prefb, maxb, post);
}

ArpUniDimens::ArpUniDimens(const ArpUniDimens& o)
{
	(*this) = o;
}

ArpUniDimens::~ArpUniDimens()
{
}

ArpUniDimens& ArpUniDimens::operator=(const ArpUniDimens& o)
{
	SetTo(o.mPreLabel, o.mMinBody, o.mPrefBody, o.mMaxBody, o.mPostLabel);
	return *this;
}

void ArpUniDimens::Init()
{
	mPreLabel = 0;
	mMinBody = 0;
	mPrefBody = 0;
	mMaxBody = ArpAnySize;
	mPostLabel = 0;
	
	ArpASSERT( validate() );
}

void ArpUniDimens::SetTo(float pre, float minb, float prefb, float maxb, float post)
{
	ArpVALIDATE(pre >= 0, pre = 0);
	ArpVALIDATE(post >= 0, post = 0);
	
	mPreLabel = pre;
	mPostLabel = post;
	
	SetBody(minb, prefb, maxb);
}

void ArpUniDimens::SetPreLabel(float value)
{
	ArpVALIDATE(value >= 0, value = 0);
	mPreLabel = value;
	ArpASSERT( validate() );
}

float ArpUniDimens::PreLabel() const
{
	return mPreLabel;
}

void ArpUniDimens::SetMinBody(float value)
{
	ArpVALIDATE(value >= 0, value = 0);
	mMinBody = value;
	ArpASSERT( validate() );
}

float ArpUniDimens::MinBody() const
{
	return mMinBody;
}

void ArpUniDimens::SetPrefBody(float value)
{
	ArpVALIDATE(value >= 0, value = 0);
	mPrefBody = value;
	ArpASSERT( validate() );
}

float ArpUniDimens::PrefBody() const
{
	return mPrefBody;
}

void ArpUniDimens::SetMaxBody(float value)
{
	ArpVALIDATE(value >= 0, value = 0);
	mMaxBody = value;
	if( mMaxBody > ArpAnySize ) mMaxBody = ArpAnySize;
	ArpASSERT( validate() );
}

float ArpUniDimens::MaxBody() const
{
	return mMaxBody;
}

void ArpUniDimens::SetPostLabel(float value)
{
	ArpVALIDATE(value >= 0, value = 0);
	mPostLabel = value;
	ArpASSERT( validate() );
}

float ArpUniDimens::PostLabel() const
{
	return mPostLabel;
}

void ArpUniDimens::SetBody(float minb, float prefb, float maxb)
{
	ArpVALIDATE(minb >= 0, minb = 0);
	ArpVALIDATE(prefb >= 0, prefb = 0);
	ArpVALIDATE(maxb >= 0, maxb = 0);
	
	mMinBody = minb;
	mPrefBody = prefb;
	mMaxBody = maxb;
	
	if( mMaxBody > ArpAnySize ) mMaxBody = ArpAnySize;
	
	ArpASSERT( validate() );
}

float ArpUniDimens::TotalMin() const
{
	const float val = mPreLabel + mMinBody + mPostLabel;
	if( val > ArpAnySize ) return ArpAnySize;
	return val;
}

float ArpUniDimens::TotalPref() const
{
	const float val = mPreLabel + mPrefBody + mPostLabel;
	if( val > ArpAnySize ) return ArpAnySize;
	return val;
}

float ArpUniDimens::TotalMax() const
{
	const float val = mPreLabel + mMaxBody + mPostLabel;
	if( val > ArpAnySize ) return ArpAnySize;
	return val;
}

void ArpUniDimens::AddBody(float value)
{
	mMinBody += value;
	if( mMinBody < 0 ) mMinBody = 0;
	mPrefBody += value;
	if( mPrefBody < 0 ) mPrefBody = 0;
	if( !IsAnySize(mMaxBody) ) {
		mMaxBody += value;
		if( mMaxBody < 0 ) mMaxBody = 0;
	}
	ArpASSERT( validate() );
}

void ArpUniDimens::AddBody(const ArpUniDimens& dim)
{
	mMinBody += dim.MinBody();
	mPrefBody += dim.PrefBody();
	mMaxBody += dim.MaxBody();
	if( mMaxBody > ArpAnySize ) mMaxBody = ArpAnySize;
	ArpASSERT( validate() );
}

void ArpUniDimens::AddLabel(float pre, float post)
{
	mPreLabel += pre;
	if( mPreLabel < 0 ) mPreLabel = 0;
	mPostLabel += post;
	if( mPostLabel < 0 ) mPostLabel = 0;
	ArpASSERT( validate() );
}

void ArpUniDimens::MakeAllBody()
{
	const float label = mPreLabel+mPostLabel;
	mPreLabel = mPostLabel = 0;
	mMinBody += label;
	mPrefBody += label;
	mMaxBody += label;
	if( mMaxBody > ArpAnySize ) mMaxBody = ArpAnySize;
	ArpASSERT( validate() );
}

void ArpUniDimens::SetMinTotal(float value)
{
	float tot = TotalMin();
	if( tot < value ) {
		mMinBody += value-tot;
		tot = TotalPref();
		if( tot < value ) {
			mPrefBody += value-tot;
			tot = TotalMax();
			if( tot < value ) {
				mMaxBody += value-tot;
			}
		}
	}
	ArpASSERT( validate() );
}

void ArpUniDimens::SetMinDimens(const ArpUniDimens& dim)
{
	mPreLabel = std::max(mPreLabel, dim.PreLabel());
	mMinBody = std::max(mMinBody, dim.MinBody());
	mPrefBody = std::max(mPrefBody, dim.PrefBody());
	mMaxBody = std::max(mMaxBody, dim.MaxBody());
	mPostLabel = std::max(mPostLabel, dim.PostLabel());
	ArpASSERT( validate() );
}

void ArpUniDimens::SetMaxDimens(const ArpUniDimens& dim)
{
	mPreLabel = std::min(mPreLabel, dim.PreLabel());
	mMinBody = std::min(mMinBody, dim.MinBody());
	mPrefBody = std::min(mPrefBody, dim.PrefBody());
	mMaxBody = std::min(mMaxBody, dim.MaxBody());
	mPostLabel = std::min(mPostLabel, dim.PostLabel());
	ArpASSERT( validate() );
}

bool ArpUniDimens::validate() const
{
	ArpVALIDATE( mPreLabel < ArpAnySize, return false );
	ArpVALIDATE( mMinBody < ArpAnySize, return false );
	ArpVALIDATE( mPrefBody < ArpAnySize, return false );
	ArpVALIDATE( mPostLabel < ArpAnySize, return false );
	ArpVALIDATE( mPostLabel <= ArpAnySize, return false );
	
	ArpVALIDATE( mPreLabel >= 0, return false );
	ArpVALIDATE( mMinBody >= 0, return false );
	ArpVALIDATE( mPrefBody >= 0, return false );
	ArpVALIDATE( mMaxBody >= 0, return false );
	ArpVALIDATE( mPostLabel >= 0, return false );
	
	ArpVALIDATE( mMinBody <= mPrefBody, return false );
	ArpVALIDATE( mPrefBody <= mMaxBody, return false );
	
	return true;
}

// --------------------------------------------------------------------------
//                                ArpDimens
// --------------------------------------------------------------------------

ArpDimens::ArpDimens()
{
}

ArpDimens::ArpDimens(const ArpUniDimens& x, const ArpUniDimens& y)
	: mX(x), mY(y)
{
}

ArpDimens::ArpDimens(const ArpDimens& o)
	: mX(o.X()), mY(o.Y())
{
}

ArpDimens::~ArpDimens()
{
}

ArpDimens& ArpDimens::operator=(const ArpDimens& o)
{
	mX = o.mX;
	mY = o.mY;
	return *this;
}

void ArpDimens::Init()
{
	mX.Init();
	mY.Init();
}

void ArpDimens::SetTo(const ArpUniDimens& x, const ArpUniDimens& y)
{
	mX = x;
	mY = y;
}

void ArpDimens::SetX(const ArpUniDimens& dimens)
{
	mX = dimens;
}

const ArpUniDimens& ArpDimens::X() const
{
	return mX;
}

ArpUniDimens& ArpDimens::X()
{
	return mX;
}

void ArpDimens::SetY(const ArpUniDimens& dimens)
{
	mY = dimens;
}

const ArpUniDimens& ArpDimens::Y() const
{
	return mY;
}

ArpUniDimens& ArpDimens::Y()
{
	return mY;
}
