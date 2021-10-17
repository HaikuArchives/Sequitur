/* AmNoteRenderer.cpp
 */
#include <cstdlib>
#include <cstdio>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmTrack.h"

#include "AmStdFactory/AmNoteRenderer.h"

/*************************************************************************
 * AM-NOTE-RENDERER
 *************************************************************************/
AmNoteRenderer::AmNoteRenderer(	const AmTimeConverter& mtc)
		: mMtc(mtc),
		  mNoteTableHeight(0)
{
	for (uint32 k=0; k<128; k++) mNoteTable[k] = 0;
}

AmNoteRenderer::~AmNoteRenderer()
{
}

void AmNoteRenderer::BeginTrack(BRect clip,
								BView* view,
								const AmTrack* track,
									ArpLineArrayCache& lineCache)
{
	for (uint32 k=0; k<mRanges.size(); k++) mRanges[k].MakeInvalid();
}

void AmNoteRenderer::DrawPhrase(BRect clip,
								BView* view,
								const AmTrack* track,
								const AmPhraseEvent* event,
								AmTime end,
								AmPhraseEvent* topPhrase,
								ArpLineArrayCache& lineCache)
{
	ArpASSERT(event && event->Phrase() && topPhrase);
	if (!event || !event->Phrase() ) return;
	if (clip.Height() != mNoteTableHeight) GenerateNoteTable( clip.Height() );
	
	const AmPhrase*		phrase = event->Phrase();
	AmTime				phraseEnd = event->EndTime();
	/* Draw the events.
	 */
	AmNode*				n = phrase->HeadNode();
	if (n) {
		DrawEvents(clip, view, track, n, end < phraseEnd ? end : phraseEnd, topPhrase, lineCache);
	}
}

void AmNoteRenderer::EndTrack(	BRect clip,
								BView* view,
								const AmTrack* track,
								ArpLineArrayCache& lineCache)
{
	/* Flush out any notes that still need to be drawn.
	 */
	for (uint32 k=0; k<mRanges.size(); k++) {
		if (mRanges[k].IsValid() )
			lineCache.AddLine(	BPoint( mRanges[k].start, clip.top + k),
								BPoint( mRanges[k].end, clip.top + k),
								view->HighColor() );
	}
}

AmPhraseRendererI* AmNoteRenderer::Copy() const
{
	return new AmNoteRenderer(mMtc);
}

void AmNoteRenderer::DrawEvents(BRect clip,
								BView* view,
								const AmTrack* track,
								AmNode* n,
								AmTime end,
								AmPhraseEvent* topPhrase,
								ArpLineArrayCache& lineCache)
{
	ArpASSERT(n && topPhrase);
	if (!n || !topPhrase) return;
	AmRange		eventRange = topPhrase->EventRange( n->Event() );
	while (n && (eventRange.start <= end) ) {
		if( n->Event()->Type() == n->Event()->PHRASE_TYPE ) {
			DrawPhrase(clip, view, track, dynamic_cast<AmPhraseEvent*>( n->Event() ), end, topPhrase, lineCache);
		} else if (n->Event()->Type() == n->Event()->NOTEON_TYPE) {
			DrawEvent(clip, view, dynamic_cast<AmNoteOn*>( n->Event() ), eventRange, lineCache);
		}
		n = n->next;
		if (n) eventRange = topPhrase->EventRange( n->Event() );
	}
}

void AmNoteRenderer::DrawEvent(	BRect clip, BView* view,
								const AmNoteOn* noteOn, AmRange eventRange,
								ArpLineArrayCache& lineCache)
{
	mTmpRange.start = mMtc.TickToPixel(eventRange.start); 
	mTmpRange.end = mMtc.TickToPixel(eventRange.end);
	uint32		index = (uint32)( mNoteTable[noteOn->Note()] );
	ArpASSERT(index < mRanges.size() );
	if ( index >= mRanges.size() ) return;
	if ( !mRanges[index].IsValid() ) {
		mRanges[index] = mTmpRange;
	} else {
		if (mRanges[index].Touches(mTmpRange) ) {
			mRanges[index] += mTmpRange;
		} else {
			lineCache.AddLine(	BPoint(mRanges[index].start, clip.top + index),
								BPoint(mRanges[index].end, clip.top + index),
								view->HighColor() );
			mRanges[index] = mTmpRange;
		}
	}
}

void AmNoteRenderer::GenerateNoteTable(float newHeight)
{
	ArpASSERT(newHeight != mNoteTableHeight);
	ArpASSERT(newHeight >= 1);
	float	top = 0;
	float	scale = 128 / newHeight;
	for (uint32 k=0; k<128; k++) {
		mNoteTable[k] = newHeight - (k / scale);
		if (mNoteTable[k] < top) mNoteTable[k] = top;
		if (mNoteTable[k] > newHeight) mNoteTable[k] = newHeight;
	}
	mRanges.resize(newHeight + 1);
	mNoteTableHeight = newHeight;
}

/*************************************************************************
 * _FLOAT-RANGE
 *************************************************************************/
_FloatRange::_FloatRange()
		: start(-1), end(-1)
{
}

bool _FloatRange::IsValid() const
{
	return (start >= 0) && (end >= 0);
}

void _FloatRange::MakeInvalid()
{
	start = -1;
	end = -1;
}

bool _FloatRange::Touches(const _FloatRange& r) const
{
	if( end < (r.start - 1) ) return false;
	if( start > (r.end + 1) ) return false;
	return true;
}

_FloatRange& _FloatRange::operator=(const _FloatRange& r)
{
	start = r.start;
	end = r.end;
	return *this;
}

_FloatRange& _FloatRange::operator+=(const _FloatRange& r)
{
	start = (start < r.start) ? start : r.start;
	end = (end > r.end) ? end : r.end;
	return *this;
}
