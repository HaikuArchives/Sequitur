/* AmDeleteService.cpp
 */
#include <stdio.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmDeleteService.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

/*************************************************************************
 * AM-DELETE-SERVICE
 *************************************************************************/
AmDeleteService::AmDeleteService()
{
}

AmDeleteService::~AmDeleteService()
{
}

void AmDeleteService::Prepare(AmSong* song)
{
}

void AmDeleteService::Finished(AmSong* song, const char* deleteName)
{
	if ( song && song->UndoContext() ) {
		BString		undoName(deleteName);
		undoName << " Events";
		song->UndoContext()->SetUndoName( undoName.String() );
		song->UndoContext()->CommitState();
	}
}

bool static is_first_tempo(AmPhrase* container, AmEvent* event)
{
	ArpASSERT(event);
	ArpASSERT(container);
	if (!container) return false;
	if (event->Type() != event->TEMPOCHANGE_TYPE) return false;
	AmNode*		head = container->HeadNode();
	if (!head) return false;
	return head->Event() == event;
}

void AmDeleteService::Delete(	AmTrack* track,
								AmEvent* event,
								AmPhraseEvent* container)
{
	ArpASSERT(track && event && container);
	/* I hate this, but it's a special case for tempo events -- we
	 * shouldn't be able to cut the first one.  Ideally this should
	 * be handled by the tool target, but this gets utilized by
	 * clients that don't (and shouldn't) have a tool target, so I
	 * can't go that route.
	 */
	if (is_first_tempo(container->Phrase(), event) ) return;

	AmTime		oldSongTime = track->Song()->CountEndTime();

	if (track->RemoveEvent(container->Phrase(), event) != B_OK) return;
	/* Remove the container if it's now empty.
	 */
	if ( container->IsEmpty() ) {
		track->RemoveEvent(NULL, container);
	}
	AmTime		newSongTime = track->Song()->CountEndTime();
	if( oldSongTime != newSongTime ) track->Song()->EndTimeChangeNotice( newSongTime );
}
