/* AmTrackHandler.cpp
 */
#define _BUILDING_AmKernel 1

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmTrack.h"
#include "AmPublic/AmTrackHandler.h"

// A convenience -- answer true if the rect is valid, false otherwise
static bool valid_rect(BRect r)
{
	return ( (r.left != -1) && (r.top != -1) && (r.top != -1) && (r.bottom != -1) );
}

/***************************************************************************
 * AM-TRACK-HANDLER
 ****************************************************************************/
AmTrackHandler::AmTrackHandler(	const AmTimeConverter& mtc, BView* target,
								float leftFudge, float rightFudge)
		: mView(target), mMtc(mtc), mChangeRect(-1, -1, -1, -1),
		  mLeftFudge(leftFudge), mRightFudge(rightFudge)
{
}

AmTrackHandler::~AmTrackHandler()
{
}

BView* AmTrackHandler::View()
{
	return mView;
}

void AmTrackHandler::SetView(BView* view, float leftFudge, float rightFudge)
{
	mView = view;
	mLeftFudge = leftFudge;
	mRightFudge = rightFudge;
}

const AmTimeConverter& AmTrackHandler::TimeConverter() const
{
	return mMtc;
}

void AmTrackHandler::GetFudgeFactor(float* leftFudge, float* rightFudge) const
{
	*leftFudge = mLeftFudge;
	*rightFudge = mRightFudge;
}

bool AmTrackHandler::HandleMessage(const BMessage* msg)
{
	return HandleTrackMessage(msg);
}

static bool has_signature_change(const BMessage* msg)
{
	ArpASSERT(msg);
	if (msg->what == AmNotifier::SIGNATURE_OBS) return true;
	int32		i;
	for (int32 k = 0; msg->FindInt32(RANGE_ALL_EVENT_STR, k, &i) == B_OK; k++) {
		if (i == AmNotifier::SIGNATURE_OBS) return true;
	}
	return false;
}

bool AmTrackHandler::HandleTrackMessage(const BMessage* msg)
{
	if (msg->what != AmNotifier::NOTE_OBS
			&& msg->what != AmNotifier::CONTROL_CHANGE_OBS
			&& msg->what != AmNotifier::PITCH_OBS
			&& msg->what != AmNotifier::SIGNATURE_OBS
			&& msg->what != AmNotifier::TEMPO_OBS
			&& msg->what != AmNotifier::OTHER_EVENT_OBS
			&& msg->what != AmNotifier::RANGE_OBS)
		return false;

	if (!mView) return true;
	BWindow*		win = mView->Window();
	if (!win) return true;
	if (win->IsHidden() || win->IsMinimized()) return true;
	
	int32	start, end;
	if (msg->FindInt32("start_time", &start) != B_OK) return true;
	if (msg->FindInt32("end_time", &end) != B_OK) return true;
	BRect	b = mView->Bounds();
	if (mLeftFudge != AM_FUDGE_TO_EDGE)
		b.left = mMtc.TickToPixel(start) - 2 - mLeftFudge;
	/* If the measure is a signature message, than leave the right
	 * bound alone -- i.e., the signature should invalidate the
	 * start of the signature til the right edge of the view.
	 */
	if (!has_signature_change(msg) && mRightFudge != AM_FUDGE_TO_EDGE)
		b.right = mMtc.TickToPixel(end) + 3 + mRightFudge;		
	if (b.left < 0) b.left = 0;
	mView->Invalidate(b);
	return true;
}

void AmTrackHandler::MergeChangeEvent(const AmEvent* event, const AmPhraseEvent& topPhrase)
{
	ArpASSERT(event);
	if ( !IsInteresting(event) ) return;
	BRect	r = RectFor(event, topPhrase.EventRange(event) );
	if ( !valid_rect(r) ) return;
		
	// If the change rect is invalid, replace it.  Otherwise, merge with it.
	if ( !valid_rect(mChangeRect) ) mChangeRect.Set(r.left, r.top, r.right, r.bottom);
	else mChangeRect = mChangeRect|r;
}

void AmTrackHandler::MergeChangePhrase(const AmPhrase* phrase, const AmPhraseEvent& topPhrase)
{
	ArpASSERT(phrase);
	AmNode*			node = phrase->HeadNode();
	while (node != 0) {
		MergeChangeEvent(node->Event(), topPhrase);
		node = node->next;
	}
}

void AmTrackHandler::DrawChangeEvent(const AmEvent* event, const AmPhraseEvent& topPhrase, bool drawImmediately)
{
	if (event) MergeChangeEvent(event, topPhrase);
	FlushChangeRect(drawImmediately);
}

void AmTrackHandler::DrawChangePhrase(const AmPhrase* phrase, const AmPhraseEvent& topPhrase, bool drawImmediately)
{
	if (phrase) MergeChangePhrase(phrase, topPhrase);
	FlushChangeRect(drawImmediately);
}

void AmTrackHandler::FlushChangeRect(bool drawImmediately)
{
	if ( !mView ) return;
	if ( valid_rect( mChangeRect ) ) {
		/* If the left bounds are below zero, that can cause a problem
		 * for some methods in AmPhrase, so I avoid that.
		 */
		float		left = mChangeRect.left - mLeftFudge;
		if( left < 0 ) left = 0;
		mView->Invalidate( BRect(	left,
									mChangeRect.top,
									mChangeRect.right + mRightFudge,
									mChangeRect.bottom ) );
		if ( drawImmediately && mView->Window() ) mView->Window()->UpdateIfNeeded();
		mChangeRect.Set(-1, -1, -1, -1);
	}
}
