/* AmToolTarget.cpp
*/
#define _BUILDING_AmKernel 1

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <be/interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmTrack.h"

/***************************************************************************
 * AM-TOOL-TARGET
 ****************************************************************************/
AmToolTarget::AmToolTarget(	AmTrackWinPropertiesI& trackWinProps,
							BView* view, float leftFudge, float rightFudge)
		: inherited(trackWinProps.TimeConverter(), view, leftFudge, rightFudge),
		  mTrackWinProps(trackWinProps)
{
}

AmToolTarget::~AmToolTarget()
{
}

AmTrackWinPropertiesI& AmToolTarget::TrackWinProperties() const
{
	return mTrackWinProps;
}

uint32 AmToolTarget::Flags() const
{
	return 0;
}

AmEvent* AmToolTarget::EventAt(	const AmTrack* track,
								BPoint where,
								AmPhraseEvent** topPhraseAnswer,
								int32* extraData)
{
	ArpASSERT(track);
	return EventAt(track, mMtc.PixelToTick(where.x), where.y, topPhraseAnswer, extraData);
}

AmEvent* AmToolTarget::RecurseEventAt(	const AmTrack* track,
										const AmPhraseEvent& topPhrase,
										const AmPhraseEvent& phrase,
										AmTime time, float y,
										int32* extraData) const
{
	ArpASSERT(phrase.Phrase());
	AmNode*				n = phrase.Phrase()->FindNode(time + EventAtFudge(), BACKWARDS_SEARCH);
	if (!n) return NULL;
	AmPhraseEvent*		pe;
	AmEvent*			event;
	while (n) {
		if( (n->Event()->Type() == n->Event()->PHRASE_TYPE)
				&& (pe = dynamic_cast<AmPhraseEvent*>( n->Event() )) ) {
			if (pe->Phrase() ) {
				event = InterestingEventAt(track, topPhrase, *(pe->Phrase()), time, y, extraData);
				if (event) return pe;

				event = RecurseEventAt(track, topPhrase, *pe, time, y, extraData);
				if (event) return event;
			}
		}
		n = n->prev;
	}
	return NULL;
}

AmEvent* AmToolTarget::EventAt(	const AmTrack* track,
								AmTime time,
								float y,
								AmPhraseEvent** topPhraseAnswer,
								int32* extraData)
{
	ArpASSERT(track);
	const AmPhrase&		phrase = track->Phrases();
	AmNode*				n = phrase.FindNode(time + EventAtFudge(), BACKWARDS_SEARCH);
	if (!n) return NULL;
	AmPhraseEvent*		pe;
	AmEvent*			event;
	while (n) {
		if( (n->Event()->Type() == n->Event()->PHRASE_TYPE)
				&& (pe = dynamic_cast<AmPhraseEvent*>( n->Event() )) ) {
			if (pe->Phrase() ) {
				event = InterestingEventAt(track, *pe, *(pe->Phrase()), time, y, extraData);
				if (event) {
					*topPhraseAnswer = pe;
					return event;
				}
				event = RecurseEventAt(track, *pe, *pe, time, y, extraData);
				if (event) {
					*topPhraseAnswer = pe;
					return event;
				}
			}
		}
		n = n->prev;
	}
	return NULL;
}

AmTime AmToolTarget::EventAtFudge() const
{
	return 0;
}

AmPhraseEvent* AmToolTarget::PhraseEventNear(const AmTrack* track, AmTime time)
{
	ArpASSERT(track);
	if (Flags()&NO_CONTAINER) {
		AmNode*		head = track->Phrases().HeadNode();
		if (!head) return NULL;
		return dynamic_cast<AmPhraseEvent*>( head->Event() );
	}
	return AmGlobals().PhraseEventNear(track, time);
}

bool AmToolTarget::EventIntersects(	const AmEvent* event, AmRange eventRange,
									AmTime left, int32 top,
									AmTime right, int32 bottom) const
{
	ArpASSERT(event);
	if( (eventRange.start > right) || (eventRange.end < left) ) return false;
	return true;
}

void AmToolTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
}

void AmToolTarget::Stop()
{
}

void AmToolTarget::PrepareForPaste(AmEvent* event)
{
}

/* ----------------------------------------------------------------
   am_trans_params Implementation
   ---------------------------------------------------------------- */

am_trans_params::am_trans_params()
	: size(sizeof(am_trans_params)),
	  flags(0), original_x(0), original_y(0),
	  delta_x(0), delta_y(0), extra_data(0)
{
}

am_trans_params::am_trans_params(const am_trans_params& o)
	: size(sizeof(am_trans_params)),
	  flags(o.flags), original_x(o.original_x), original_y(o.original_y),
	  delta_x(o.delta_x), delta_y(o.delta_y), extra_data(o.extra_data)
{
}

am_trans_params::~am_trans_params()
{
}

am_trans_params& am_trans_params::operator=(const am_trans_params& o)
{
	if (this != &o) memcpy(this, &o, sizeof(o));
	return *this;
}
