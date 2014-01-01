/* AmGraphicEffects.cpp
 */
#include <stdio.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmGraphicEffects.h"

//static int32	gCount = 0;

/*************************************************************************
 * AM-GRAPHIC-EFFECT
 *************************************************************************/
AmGraphicEffect::AmGraphicEffect(tool_id ownerId)
		: mOwnerId(ownerId)
{
//	gCount++;
//	printf("Construct tool graphic %ld\n", gCount);
}

AmGraphicEffect::~AmGraphicEffect()
{
//	printf("Destruct tool graphic %ld\n", gCount);
//	gCount--;
}

status_t AmGraphicEffect::GetEffectInfo(uint32 index,
										BString& outLabel,
										BString& outKey)
{
	if (index == 0) {
		outLabel = "Fading Line";
		outKey = "arp:FadingLine";
		return B_OK;
	} else return B_ERROR;
}

status_t AmGraphicEffect::GetEffectInfo(const BString& key,
										BString& outLabel)
{
	if (key == "arp:FadingLine") {
		outLabel = "Fading Line";
		return B_OK;
	} else return B_ERROR;
}

AmGraphicEffect* AmGraphicEffect::NewEffect(const BString& key, tool_id id)
{
	if (key == "arp:FadingLine") return new AmFadingLineEffect(id);
	else return NULL;
}

tool_id AmGraphicEffect::OwnerId()
{
	return mOwnerId;
}

void AmGraphicEffect::_ReservedAmGraphicEffect1() { }
void AmGraphicEffect::_ReservedAmGraphicEffect2() { }
void AmGraphicEffect::_ReservedAmGraphicEffect3() { }
void AmGraphicEffect::_ReservedAmGraphicEffect4() { }
void AmGraphicEffect::_ReservedAmGraphicEffect5() { }
void AmGraphicEffect::_ReservedAmGraphicEffect6() { }
void AmGraphicEffect::_ReservedAmGraphicEffect7() { }
void AmGraphicEffect::_ReservedAmGraphicEffect8() { }
void AmGraphicEffect::_ReservedAmGraphicEffect9() { }
void AmGraphicEffect::_ReservedAmGraphicEffect10() { }
void AmGraphicEffect::_ReservedAmGraphicEffect11() { }
void AmGraphicEffect::_ReservedAmGraphicEffect12() { }
void AmGraphicEffect::_ReservedAmGraphicEffect13() { }
void AmGraphicEffect::_ReservedAmGraphicEffect14() { }
void AmGraphicEffect::_ReservedAmGraphicEffect15() { }
void AmGraphicEffect::_ReservedAmGraphicEffect16() { }

/*************************************************************************
 * AM-FADING-LINE-EFFECT
 *************************************************************************/
static const uint32		FADE_MSG	= 'aflM';

AmFadingLineEffect::AmFadingLineEffect(tool_id ownerId, uint32 maxLines)
		: inherited(ownerId), BMessageFilter(FADE_MSG),
		  mTarget(NULL), mMaxLines(maxLines), mHasLastPoint(false),
		  mFadeRunner(NULL)
{
	SetFadeColor( Prefs().Color(AM_DATA_BG_C) );
	SetLineColor( Prefs().Color(AM_DATA_FG_C) );
}

AmFadingLineEffect::~AmFadingLineEffect()
{
	CleanUp();
}

bool AmFadingLineEffect::IsFinished() const
{
	return mLines.size() == 0;
}

void AmFadingLineEffect::Begin(BView* target, BPoint pt)
{
	mLastPoint = pt;
	mHasLastPoint = true;

	if (!mTarget && target) {
		mTarget = target;
		mTarget->AddFilter(this);
		BMessage	msg(FADE_MSG);
		mFadeRunner = new BMessageRunner( BMessenger(mTarget), &msg, 200000, -1);
	}
}

void AmFadingLineEffect::MouseMoved(BPoint where, uint32 code)
{
	if (!mHasLastPoint) {
		mLastPoint = where;
		mHasLastPoint = true;
	} else {
		AddLine(mLastPoint, where, mLineColor);
		mLastPoint = where;
	}
}

void AmFadingLineEffect::DrawOn(BView* view, BRect clip)
{
	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->SetLowColor( view->ViewColor() );
	
	for (uint32 k = 0; k < mLines.size(); k++) {
		view->SetHighColor(mLines[k].c);
		view->StrokeLine(mLines[k].from, mLines[k].to);
	}

	view->SetDrawingMode(mode);
}

filter_result AmFadingLineEffect::Filter(BMessage *message, BHandler **target)
{
	if (message && message->what == FADE_MSG
			&& mTarget && mTarget == *target) {
		AlphaFade();
	}
	return B_DISPATCH_MESSAGE;
}

void AmFadingLineEffect::AddLine(BPoint from, BPoint to, rgb_color c)
{
	BRect	r(	min(from.x, to.x), min(from.y, to.y),
				max(from.x, to.x), max(from.y, to.y) );
	if (mLines.size() >= mMaxLines) {
		/* Fix: merge area in with rect.
		 */
		mLines.erase( mLines.begin() );
	}
	mLines.push_back( _AmLineSegment(from, to, c) );
	if (mTarget) {
		mTarget->Invalidate(r);
	}
}

void AmFadingLineEffect::Fade(uint8 step)
{
	BRect		invalid;
	bool		unset = true;
	for (int32 k = (int32)mLines.size() - 1; k >= 0; k--) {
		mLines[k].Fade(step, mFadeColor);

		float	left = min(mLines[k].from.x, mLines[k].to.x);
		float	top = min(mLines[k].from.y, mLines[k].to.y);
		float	right = max(mLines[k].from.x, mLines[k].to.x);
		float	bottom = max(mLines[k].from.y, mLines[k].to.y);
		if (unset) {
			invalid.Set(left, top, right, bottom);
			unset = false;
		} else {
			if (left < invalid.left) invalid.left = left;
			if (top < invalid.top) invalid.top = top;
			if (right > invalid.right) invalid.right = right;
			if (bottom > invalid.bottom) invalid.bottom = bottom;
		}

		if (mLines[k].c.red == mFadeColor.red
				&& mLines[k].c.green == mFadeColor.green
				&& mLines[k].c.blue == mFadeColor.blue) {
			mLines.erase(mLines.begin() + k);
		}
	}
	if (!unset && mTarget) mTarget->Invalidate(invalid);
}

void AmFadingLineEffect::AlphaFade(uint8 step)
{
	BRect		invalid;
	bool		unset = true;
	for (int32 k = (int32)mLines.size() - 1; k >= 0; k--) {
		mLines[k].AlphaFade(step);

		float	left = min(mLines[k].from.x, mLines[k].to.x);
		float	top = min(mLines[k].from.y, mLines[k].to.y);
		float	right = max(mLines[k].from.x, mLines[k].to.x);
		float	bottom = max(mLines[k].from.y, mLines[k].to.y);
		if (unset) {
			invalid.Set(left, top, right, bottom);
			unset = false;
		} else {
			if (left < invalid.left) invalid.left = left;
			if (top < invalid.top) invalid.top = top;
			if (right > invalid.right) invalid.right = right;
			if (bottom > invalid.bottom) invalid.bottom = bottom;
		}

		if (mLines[k].c.alpha == 0) {
			mLines.erase(mLines.begin() + k);
		}
	}
	if (!unset && mTarget) mTarget->Invalidate(invalid);
}

void AmFadingLineEffect::SetFadeColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	mFadeColor.red = r;
	mFadeColor.green = g;
	mFadeColor.blue = b;
	mFadeColor.alpha = a;
}

void AmFadingLineEffect::SetFadeColor(rgb_color c)
{
	SetFadeColor(c.red, c.green, c.blue, c.alpha);
}

void AmFadingLineEffect::SetLineColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	mLineColor.red = r;
	mLineColor.green = g;
	mLineColor.blue = b;
	mLineColor.alpha = a;
}

void AmFadingLineEffect::SetLineColor(rgb_color c)
{
	SetLineColor(c.red, c.green, c.blue, c.alpha);
}

void AmFadingLineEffect::CleanUp()
{
	if (mTarget) mTarget->RemoveFilter(this);
	delete mFadeRunner;
	mFadeRunner = NULL;
}

/***************************************************************************
 * _AM-LINE-SEGMENT
 ***************************************************************************/

_AmLineSegment::_AmLineSegment(BPoint From, BPoint To, rgb_color C)
		: from(From), to(To), c(C)
{
}

_AmLineSegment& _AmLineSegment::operator=(const _AmLineSegment &o)
{
	from = o.from;
	to = o.to;
	c = o.c;
	return *this;
}

void _AmLineSegment::Fade(uint8 step, rgb_color fadeC)
{
	if (c.red < fadeC.red) {
		if ( (int32)c.red + (int32)step  >= fadeC.red ) c.red = fadeC.red;
		else c.red += step;
	} else if (c.red > fadeC.red) {
		if ( (int32)c.red - (int32)step  >= fadeC.red ) c.red = fadeC.red;
		else c.red -= step;
	}

	if (c.green < fadeC.green) {
		if ( (int32)c.green + (int32)step  >= fadeC.green ) c.green = fadeC.green;
		else c.green += step;
	} else if (c.green > fadeC.green) {
		if ( (int32)c.green - (int32)step  >= fadeC.green ) c.green = fadeC.green;
		else c.green -= step;
	}

	if (c.blue < fadeC.blue) {
		if ( (int32)c.blue + (int32)step  >= fadeC.blue ) c.blue = fadeC.blue;
		else c.blue += step;
	} else if (c.blue > fadeC.blue) {
		if ( (int32)c.blue - (int32)step  >= fadeC.blue ) c.blue = fadeC.blue;
		else c.blue -= step;
	}
}

void _AmLineSegment::AlphaFade(uint8 step)
{
	int32	n = c.alpha - step;
	if (n <= 0) c.alpha = 0;
	else c.alpha -= step;
}
