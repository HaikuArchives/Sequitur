/* SeqPhraseTool.cpp
 */
#include <algo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/Window.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmSongObserver.h"

#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

#include "Sequitur/SeqPhraseTool.h"
#include "Sequitur/SeqSongSelections.h"
#include "Sequitur/SeqSongToolTarget.h"
#include "Sequitur/SequiturDefs.h"

/* The number of pixels the mouse has to move before a drag is initiated.
 */
static const float	INITIATE_DRAG_DISTANCE	= 4;
static const uint32	DRAG_SELECTION_MSG		= 'drgs';
static const char*	START_STR				= "start";
static const char*	END_STR					= "end";
static const char*	BUTTONS_STR				= "buttons";
static const char*	MEASURE_START_STR		= "measure start";
static const char*	SOURCE_TRACK_STR		= "source track";
static const char*	TRACK_STR				= "track";

static const uint32 LINK_ANSWER				= 0;
static const uint32 MOVE_ANSWER				= 1;
static const uint32 COPY_ANSWER				= 2;
static const uint32 CANCEL_ANSWER			= 3;

static const uint32	I_SCROLL_MSG			= 'iscr';

/* Answer true if the two points are within distance pixels of each other.
 */
static bool within_distance(BPoint pt1, BPoint pt2, float distance)
{
	if (fabs(pt1.x - pt2.x) > distance) return false;
	if (fabs(pt1.y - pt2.y) > distance) return false;
	return true;
}

/*************************************************************************
 * _AM-DROP-ENTRY
 * This class caches information that gets built up while this tool is
 * handling a drop result.  At the end of the drop result, the information
 * in this class is used to add all the phrases to the track and send out
 * notification.
 *************************************************************************/
class _AmDropEntry
{
public:
	_AmDropEntry();
	_AmDropEntry(AmTrack* srcTrack, AmTrack* destTrack);
	_AmDropEntry(const _AmDropEntry& o);
	~_AmDropEntry();
	
	_AmDropEntry&			operator=(const _AmDropEntry &o);
	bool					IsValid() const;
	void					RemoveAndSetTime(	AmRange range, AmTime timeDelta,
												bool copy,
												int32* undoCount,
												AmEvent::EventType* undoType);
	void					MoveLink(	AmPhraseEvent* pe, AmTime timeDelta,
										bool copy, int32* undoCount, AmEvent::EventType* undoType);
	void					MovePhrase(	AmPhraseEvent* topPhrase, AmRange range, AmTime timeDelta,
										bool copy, int32* undoCount, AmEvent::EventType* undoType);
	void					MoveEvent(	AmPhraseEvent* topPhrase, AmPhraseEvent* newPhrase,
										AmEvent* event, AmTime timeDelta, bool copy,
										int32* undoCount, AmEvent::EventType* undoType);

	void					AddEvents();

	AmTrack*				mSrcTrack;
	AmTrack*				mDestTrack;
	vector<AmPhraseEvent*>	mPhrases;
};

/*************************************************************************
 * SEQ-PHRASE-TOOL
 *************************************************************************/
SeqPhraseTool::SeqPhraseTool(AmSongRef songRef, const AmTimeConverter& mtc)
		: mSongRef(songRef), mMtc(mtc),
		  mButtons(0), mOriginalPoint(0, 0),
		  mIsDrag(false), mExited(false),
		  mScrollRunner(0), mScrollDelta(0)

{
}

SeqPhraseTool::~SeqPhraseTool()
{
	delete mScrollRunner;
}

float SeqPhraseTool::ScrollDelta() const
{
	return mScrollDelta;
}

void SeqPhraseTool::SetScrollDelta(float delta)
{
	mScrollDelta = delta;
}

void SeqPhraseTool::MouseDown(SeqSongToolTarget* target, BPoint where)
{
	ArpASSERT( target && target->View() && target->View()->Window() );
//	target->View()->GetMouse(&where, &mButtons, false);
	if (target->View()->Window()->CurrentMessage()->FindInt32("buttons", &mButtons) != B_OK) mButtons = 0;
	mOriginalPoint = where;
	mIsDrag = false;
	mExited = false;
	SetScrollDelta(0);

	/* Start receiving a message that tells me to check to see if
	 * any scrolling needs to happen.
	 */
	BMessage		msg(I_SCROLL_MSG);
	mScrollRunner = new BMessageRunner( BMessenger( target->View() ), &msg, 50000, -1);

	/* If this is a double click then pass it to the window with
	 * all the information I have.
	 */
	int32 clicks = 1;
	target->View()->Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	if (mButtons&B_PRIMARY_MOUSE_BUTTON && clicks == 2) {
		track_id	trackId = 0;
		// READ SONG BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqPhraseTool::MouseDown() read lock\n"); fflush(stdout);
		#endif
		const AmSong*	song = mSongRef.ReadLock();
		if (song) {
			const AmTrack*	track = Track(song, where);
			if (track) trackId = track->Id();
		}
		mSongRef.ReadUnlock(song);
		// END READ SONG BLOCK
		if (trackId) {
			BMessage	msg(SHOWTRACKWIN_MSG);
			msg.AddPointer(SZ_TRACK_ID, trackId);
			add_time(msg, SZ_AMTIME, mMtc.PixelToTick(where.x) );
			target->View()->Window()->PostMessage(&msg);
		}
		return;
	}
	/* Otherwise, select whatever lies at the click point.
	 */
	bool	mergeOnShift = false;
	if (mButtons&B_PRIMARY_MOUSE_BUTTON) mergeOnShift = true;
	if (mButtons&B_SECONDARY_MOUSE_BUTTON) mergeOnShift = true;
	bool	selectionMade = PointSelected(target, where, mergeOnShift);
}

void SeqPhraseTool::MouseMoved(	SeqSongToolTarget* target,
								BPoint where,
								uint32 code,
								const BMessage* dragMessage)
{
	ArpASSERT(target && target->View() && target->View()->Window() );
	/* Sometimes, this view might not receive a mouse up for
	 * whatever reason, which can cause the scroll runner to
	 * lock into permanent autoscroll.  This simply makes sure
	 * the scroll runner has been deleted if it's not valid.
	 */
	if (mScrollRunner) {
		int32		buttons = 0;
		target->View()->Window()->CurrentMessage()->FindInt32("buttons", &buttons);
		if (buttons == 0) {
			delete mScrollRunner;
			mScrollRunner = NULL;
		}
	}

	if (code == B_ENTERED_VIEW || code == B_INSIDE_VIEW) mExited = false;
	if (code == B_EXITED_VIEW) mExited = true;
	
	if (dragMessage && dragMessage->what == DRAG_SELECTION_MSG) {
		Drag(target, where, code, dragMessage);
		return;
	}
	if (mButtons == 0) return;
	if (!mIsDrag) {
		if( !within_distance(mOriginalPoint, where, INITIATE_DRAG_DISTANCE) ) {
			InitiateDrag(target, where);
		}
	}
}

void SeqPhraseTool::MouseUp(SeqSongToolTarget* target, BPoint where)
{
	ArpASSERT(target);
	mButtons = 0;
	mIsDrag = false;
	delete mScrollRunner;
	mScrollRunner = 0;
	target->ClearDragMark();
}

bool SeqPhraseTool::HandleMessage(	const BMessage* msg,
									track_id trackId,
									SeqSongToolTarget* target)
{
	ArpASSERT( msg && target && target->View() && target->TimeConverter() );
	if (msg->what == I_SCROLL_MSG) {
		HandleScrollMsg(target, msg);
		return true;
	}
	
	if (msg->what != DRAG_SELECTION_MSG) return false;
	BPoint		where;
	AmRange		range;
	uint32		buttons;
	AmTime		measureStart;
	int32		sourceTrack;
	song_id		srcSongId;
	song_id		destSongId = mSongRef.SongId();
	vector<uint32> tracks;
	target->View()->GetMouse(&where, &buttons, false);
	AmTime		dropTime = target->TimeConverter()->PixelToTick( where.x );
	if( find_time( *msg, START_STR, &(range.start) ) != B_OK ) return true;
	if( find_time( *msg, END_STR, &(range.end) ) != B_OK ) return true;
	if( msg->FindInt32( BUTTONS_STR, (int32*)&buttons ) != B_OK ) buttons = 0;
	if( find_time( *msg, MEASURE_START_STR, &measureStart ) != B_OK ) return true;
	if( msg->FindInt32( SOURCE_TRACK_STR, &sourceTrack ) != B_OK ) return true;
	if( msg->FindPointer( SZ_SONG_ID, &srcSongId ) != B_OK ) return true;
	int32		trackIndex;
	for (int32 k = 0; msg->FindInt32(TRACK_STR, k, &trackIndex) == B_OK; k++) {
		tracks.push_back( uint32(trackIndex) );
	}
	if (tracks.size() < 1) return true;
	sort( tracks.begin(), tracks.end(), less<uint32>() );
	bool		copy = false;
	if (buttons&B_SECONDARY_MOUSE_BUTTON) {
		bool	enableLink = (srcSongId == destSongId);
		uint32	op = DroppedMenu(target->View()->ConvertToScreen(where), enableLink);
		if (op == CANCEL_ANSWER) return true;
		else if (op == COPY_ANSWER) copy = true;
//		else if (op == LINK_ANSWER) {
//			CreateLink(trackId, range, sourceTrack, tracks, dropTime);
//			return true;
//		}
	}
	// WRITE SONG BLOCK
	AmSong*		destSong = mSongRef.WriteLock();
	if (destSong) {
		uint32		destinationTrack = uint32(destSong->TrackIndex(trackId) );
		if (srcSongId == destSongId) {
			PerformDrop(target, destSong, destSong, range, tracks, destinationTrack - sourceTrack, dropTime, measureStart, copy);
		} else {
			AmSongRef	srcRef = AmGlobals().SongRef(srcSongId);
			AmSongObserver obs(srcRef);
			// WRITE SONG BLOCK
			AmSong*		srcSong = obs.WriteLock();
			if (srcSong) {
				// If I'm dropping onto a different song, always perform a copy
				PerformDrop(target, srcSong, destSong, range, tracks, destinationTrack - sourceTrack, dropTime, measureStart, true);
				if ( srcSong->UndoContext() ) srcSong->UndoContext()->CommitState();
			}
			obs.WriteUnlock( srcSong );
			// END WRITE SONG BLOCK
		}
		if ( destSong->UndoContext() ) destSong->UndoContext()->CommitState();
	}
	mSongRef.WriteUnlock(destSong);
	// END WRITE SONG BLOCK
	return true;
}

void SeqPhraseTool::CreateLink(	track_id trackId,
								AmRange range,
								int32 sourceTrack,
								vector<uint32>& tracks,
								AmTime dropTime)
{
	// WRITE SONG BLOCK
	AmSong*		song = mSongRef.WriteLock();
	if (!song) return;
	/* FIX: For this to work with each track having its own
	 * time signatures, I need to store the measureStart of each
	 * track in the message, supply those here, and get the measure
	 * start for each track that events are being sent to.
	 */
	AmSignature		sig;
	int32			destinationTrack = song->TrackIndex(trackId);
	if (AmGlobals().GetMeasure(song->Signatures(), dropTime, &sig) == B_OK
			&& destinationTrack >= 0) {
		AmTime			newMeasureStart = sig.StartTime();
		int32			trackDelta = destinationTrack - sourceTrack;
		AmTrack*		srcTrack;
		AmTrack*		destTrack;

		for (uint32 k = 0; k < tracks.size(); k++) {
			if( (srcTrack = song->Track( tracks[k] ))
					&& (destTrack = song->Track( tracks[k] + trackDelta )) ) {
				/* For each phrase that starts in srcTrack, create a link in destTrack
				 * at the appropriate time.
				 */
				const AmPhrase&		srcPhrases = srcTrack->Phrases();
				AmNode*				node = srcPhrases.FindNode(range.start);
				while (node && node->StartTime() <= range.end) {
					if (node->Event()->Type() == node->Event()->PHRASE_TYPE) {
						AmPhraseEvent*		pe = dynamic_cast<AmPhraseEvent*>( node->Event() );
						if (pe) {
							AmTime			newTime = newMeasureStart + (pe->StartTime() - range.start);
							AmLinkEvent*	link = new AmLinkEvent(newTime, pe->Phrase());
							if (link) destTrack->AddEvent(NULL, link);
						}
					}
					node = node->next;
				}
			}
		}
	}
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

bool SeqPhraseTool::PointSelected(	SeqSongToolTarget* target,
									BPoint where,
									bool mergeOnShift)
{
	ArpASSERT(target);
	bool					answer = false;
	SeqSongSelections*		selections = NULL;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseTool::PointSelected() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) {
		const AmTrack*	track = Track(song, where);
		if (track) {
			AmPhraseEvent*	pe = target->PhraseEventAt(track, where);
			answer = PhraseSelected(track, target, pe, &selections, mergeOnShift);
		}
	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
	if (selections) target->SetSelections(selections);
	else target->SetSelections(NULL);
	return answer;
}

bool SeqPhraseTool::PhraseSelected(	const AmTrack* track,
									SeqSongToolTarget* target,
									AmPhraseEvent* pe,
									SeqSongSelections** outSelections,
									bool mergeOnShift)
{
	SeqSongSelections*	selections = NULL;
	if( mergeOnShift && (modifiers()&B_SHIFT_KEY) ) selections = target->Selections();

	if (!selections) selections = SeqSongSelections::New();
	if (!selections) return false;

	if (!pe) {
		*outSelections = selections;
//		target->SetSelections( selections );
		return false;
	}

	MergeRange(pe, selections);
	MergeTrack(track, selections);

//	target->SetSelections( selections );
	*outSelections = selections;
	return true;
}

void SeqPhraseTool::MergeRange(AmPhraseEvent* pe, SeqSongSelections* selections) const
{
	assert( pe && selections );
	AmRange		currRange = pe->TimeRange();

	selections->SetTimeRange( currRange += selections->TimeRange() );
}

void SeqPhraseTool::MergeTrack(const AmTrack* track, SeqSongSelections* selections) const
{
	ArpASSERT(track && selections);
	if (!track || !selections) return;
	if (!selections->IncludesTrack(track->Id() ) )
		selections->AddTrack(track->Id() );
}

void SeqPhraseTool::InitiateDrag(const SeqSongToolTarget* target, BPoint where)
{
	const SeqSongSelections* selections = target->Selections();
	BView*						view = target->View();
	const AmTimeConverter*		mtc = target->TimeConverter();
	if (!selections || !view || !mtc) return;
	if (selections->IsEmpty() ) return;
	AmRange						range = selections->TimeRange();
	AmSignature					sig;
	uint32						sourceTrack = 0;
	status_t					err = B_ERROR;
	AmTime						rangeEnd = range.end;
	BMessage					msg(DRAG_SELECTION_MSG);
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseTool::InitiateDrag() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) {
		if (range.start < 0) range.start = 0;
		if (rangeEnd < 0) rangeEnd = song->CountEndTime();
		const AmTrack*	track = Track(song, where);
		if (track) {
			err = AmGlobals().GetMeasure(track->Signatures(), range.start, &sig);
			sourceTrack = uint32(song->TrackIndex( track->Id() ) );
		}
		/* Translate the selections track_id's into a bunch of track indexes.
		 * The drag-drop mechanism was originally written for track indexes,
		 * and it's not worth changing it.
		 */
		uint32			count = selections->CountTracks();
		for (uint32 k = 0; k < count; k++) {
			int32		index = song->TrackIndex(selections->TrackAt(k));
			if (index >= 0) msg.AddInt32(TRACK_STR, index);		
		}
	}
	mSongRef.ReadUnlock(song);
	// END READ TRACK BLOCK
	if (err != B_OK) return;
	
	mIsDrag = true;
	if (add_time(msg, START_STR, range.start) != B_OK) return;
	if (add_time(msg, END_STR, rangeEnd) != B_OK) return;
	if (msg.AddInt32(BUTTONS_STR, mButtons) != B_OK) return;
	if (add_time(msg, MEASURE_START_STR, sig.StartTime() ) != B_OK) return;
	if (msg.AddInt32(SOURCE_TRACK_STR, sourceTrack) != B_OK) return;
	if (msg.AddPointer(SZ_SONG_ID, mSongRef.SongId() ) != B_OK) return;
#if 0
	uint32			count = selections->CountTracks();
	for( uint32 k = 0; k < count; k++ ) {
//		if( msg.AddInt32( TRACK_STR, selections->TrackAt(k) ) != B_OK ) return;		
		if (msg.AddPointer(SZ_TRACK_ID, selections->TrackAt(k) ) != B_OK) return;		
	}
#endif

#if 0
	float		left = mtc->TickToPixel( range.start );
	float		right = mtc->TickToPixel( range.end );
	view->DragMessage( &msg, BRect(left, 0, right, 20) );
#endif
	view->DragMessage(&msg, BRect(0, 0, 0, 0) );
}

void SeqPhraseTool::Drag(	SeqSongToolTarget* target, BPoint where,
							uint32 code, const BMessage* dragMessage)
{
	if (!target || !target->View() || !target->View()->Window() || !target->TimeConverter() ) return;
	BView*		v = target->View();
	/* Scroll the arrange view if the user has dragged off the edge.
	 */
	BPoint	pWhere = v->ConvertToParent(where);
	BRect	f = v->Frame();
	float	scrollDelta = 0;
	if (pWhere.x < f.left) scrollDelta = pWhere.x;
	else if (pWhere.x > f.right) scrollDelta = pWhere.x - f.right;
	SetScrollDelta(scrollDelta);
	/* Display an indicator for where the drop will occur.
	 */
	if (code != B_EXITED_VIEW && mExited) return;
	AmSignature		sig;
	status_t		err = B_ERROR;
	AmTime			time = target->TimeConverter()->PixelToTick(where.x);
	if (time < 0) time = 0;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseTool::Drag() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) {
		const AmTrack*	track = Track(song, where);
		if (track) err = AmGlobals().GetMeasure(track->Signatures(), time, &sig);
	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
	if (code == B_EXITED_VIEW) target->ClearDragMark();
	else target->ShowDragMark(sig.StartTime(), where, dragMessage);
}

void SeqPhraseTool::PerformDrop(SeqSongToolTarget* target,
								AmSong* srcSong,
								AmSong* destSong,
								AmRange range,
								vector<uint32>& tracks,
								int32 trackDelta,
								AmTime dropTime,
								AmTime oldMeasureStart,
								bool copy)
{
	AmTime			srcEndTime = srcSong->CountEndTime();
	AmTime			destEndTime = 0;
	if (srcSong != destSong) destEndTime = destSong->CountEndTime();
	/* FIX: For this to work with each track having its own
	 * time signatures, I need to store the measureStart of each
	 * track in the message, supply those here, and get the measure
	 * start for each track that events are being sent to.
	 */
	AmSignature		sig;
	if( AmGlobals().GetMeasure( destSong->Signatures(), dropTime, &sig ) != B_OK ) return;
	AmTime			newMeasureStart = sig.StartTime();
	AmTrack*		srcTrack;
	AmTrack*		destTrack;
	vector<_AmDropEntry> entries;
	int32			undoCount = 0;
	AmEvent::EventType undoType = AmEvent::_NUM_TYPE;
	
	/* The drop happens in three stages:
	 *   1. Remove all events in the selection range and place them in a
	 *      temporary holding area.
	 *   2. Give the events new times.
	 *   3. Add the events back in.
	 */
	
	/* 1. and 2. Remove all events, set their times.
	 */ 
	for( uint32 k = 0; k < tracks.size(); k++ ) {
		if ( (srcTrack = srcSong->Track(tracks[k]))
				&& (destTrack = destSong->Track( tracks[k] + trackDelta )) ) {
			_AmDropEntry		entry(srcTrack, destTrack);
			entry.RemoveAndSetTime(range, newMeasureStart - oldMeasureStart, copy, &undoCount, &undoType);
			if (entry.IsValid() ) entries.push_back(entry);
		}
	}
	/* 3. Add the events back in.
	 */
	for( uint32 k = 0; k < entries.size(); k++ )
		if ( entries[k].IsValid() ) entries[k].AddEvents();

	/* Reset the selections to the new area.
	 */
	SeqSongSelections*	newSelections = SeqSongSelections::New();
	if (newSelections) {
		AmRange				newRange;
		newRange.start = range.start - oldMeasureStart + newMeasureStart;
		newRange.end = range.end - oldMeasureStart + newMeasureStart;
		newSelections->SetTimeRange( newRange ); 
		for (uint32 k = 0; k < tracks.size(); k++) {
			int32	newTrack = tracks[k] + trackDelta;
//			if (newTrack >= 0) newSelections->AddTrack( (uint32)newTrack );
			AmTrack*		t;
			if (newTrack >= 0 && (t = srcSong->Track(newTrack)) != NULL)
				newSelections->AddTrack(t->Id() );
		}
		target->SetSelections(newSelections);
	}

	/* Set the undo name.  This is a rather complicated affair due to the
	 * fact that if the move was across songs, then each song has a separate
	 * undo context.
	 */
	BString			undoPostfix;
	undoPostfix << undoCount << " ";
	string_for_event_type(undoPostfix, undoType, AmEvent::NO_SUBTYPE, (undoCount == 1) ? false : true);

	if (srcSong == destSong) {
		if ( srcSong->UndoContext() ) {
			BString		undoName;
			if (copy) undoName << "Copy ";
			else undoName << "Move ";
			undoName << undoPostfix;
			srcSong->UndoContext()->SetUndoName( undoName.String() );
		}
	} else {
		if ( srcSong && srcSong->UndoContext() ) {
			BString		undoName("Remove ");
			undoName << undoPostfix;
			srcSong->UndoContext()->SetUndoName( undoName.String() );
		}
		if ( destSong && destSong->UndoContext() ) {
			BString		undoName("Add ");
			undoName << undoPostfix;
			destSong->UndoContext()->SetUndoName( undoName.String() );
		}
	}

	AmTime			srcNewEndTime = srcSong->CountEndTime();
	if (srcEndTime != srcNewEndTime) srcSong->EndTimeChangeNotice(srcNewEndTime);
	if (srcSong != destSong) {
		AmTime		destNewEndTime = destSong->CountEndTime();
		if (destEndTime != destNewEndTime) destSong->EndTimeChangeNotice(destNewEndTime);
	}
}

uint32 SeqPhraseTool::DroppedMenu(BPoint where, bool enableLink) const
{
	BPopUpMenu*		menu = new BPopUpMenu("menu");
	if( !menu ) return CANCEL_ANSWER;
	menu->SetFontSize( 10 );
//	BMenuItem*		link = new BMenuItem( "Create link here", new BMessage('link') );
	BMenuItem*		move = new BMenuItem( "Move here", new BMessage('move') );
	BMenuItem*		copy = new BMenuItem( "Copy here", new BMessage('copy') );
	BMenuItem*		cancel = new BMenuItem( "Cancel", new BMessage('cncl') );
	if (!move || !copy || !cancel) {
		delete menu;
//		delete link;
		delete move;
		delete copy;
		delete cancel;
		return CANCEL_ANSWER;
	}
//	link->SetEnabled(enableLink);
//	menu->AddItem(link);
	menu->AddItem(move);
	menu->AddItem(copy);
	menu->AddSeparatorItem();
	menu->AddItem(cancel);

	BMenuItem*		answer = menu->Go( where, false, true, false );
	if ( !answer || !(answer->Message()) ) return CANCEL_ANSWER;
//	if (answer->Message()->what == 'link') return LINK_ANSWER;
	if (answer->Message()->what == 'move') return MOVE_ANSWER;
	if (answer->Message()->what == 'copy') return COPY_ANSWER;
	return CANCEL_ANSWER;
}

const AmTrack* SeqPhraseTool::Track(const AmSong* song, BPoint where) const
{
	float			top = 0;
	const AmTrack*	track = NULL;
	for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
		float		bottom = top + track->PhraseHeight();
		if (where.y >= top && where.y <= bottom) return track;
		top = bottom + 1;
	}
	return NULL;
}

void SeqPhraseTool::HandleScrollMsg(const SeqSongToolTarget* target, const BMessage* msg)
{
	ArpASSERT( target );
	float	delta = ScrollDelta();
	if( delta != 0 ) {
		BMessage	scrollMsg( 'scrA' );
		scrollMsg.AddFloat( "delta", delta );
		if( target->View() && target->View()->Window() ) {
			target->View()->Window()->PostMessage( &scrollMsg );
		}
	}
}

/*************************************************************************
 * _AM-DROP-ENTRY
 *************************************************************************/
_AmDropEntry::_AmDropEntry()
		: mSrcTrack(NULL), mDestTrack(NULL)
{
}

_AmDropEntry::_AmDropEntry(AmTrack* srcTrack, AmTrack* destTrack)
		: mSrcTrack(srcTrack), mDestTrack(destTrack)
{
}

_AmDropEntry::_AmDropEntry(const _AmDropEntry& o)
		: mSrcTrack(o.mSrcTrack), mDestTrack(o.mDestTrack)
{
	for( uint32 k = 0; k < o.mPhrases.size(); k++ )
		mPhrases.push_back( o.mPhrases[k] );
}

_AmDropEntry::~_AmDropEntry()
{
}

_AmDropEntry& _AmDropEntry::operator=(const _AmDropEntry &o)
{
	mSrcTrack = o.mSrcTrack;
	mDestTrack = o.mDestTrack;
	mPhrases.resize(0);
	for( uint32 k = 0; k < o.mPhrases.size(); k++ )
		mPhrases.push_back( o.mPhrases[k] );
	return *this;
}

bool _AmDropEntry::IsValid() const
{
	if( !mSrcTrack || !mDestTrack ) return false;
	if( mPhrases.size() < 1 ) return false;
	return true;
}

void _AmDropEntry::RemoveAndSetTime(AmRange range, AmTime timeDelta,
									bool copy,
									int32* undoCount,
									AmEvent::EventType* undoType)
{
	ArpASSERT(mSrcTrack);
	if (!mSrcTrack) return;
	const AmPhrase&		phrases = mSrcTrack->Phrases();
	AmNode*				phrase = phrases.HeadNode();
	AmNode*				nextPhrase;
	while (phrase && phrase->StartTime() <= range.end) {
		nextPhrase = phrase->next;
		AmPhraseEvent*	topPhrase = dynamic_cast<AmPhraseEvent*>( phrase->Event() );
		if (phrase->EndTime() >= range.start && topPhrase && topPhrase->Phrase()) {
			if (topPhrase->Subtype() == topPhrase->LINK_SUBTYPE)
				MoveLink(topPhrase, timeDelta, copy, undoCount, undoType);
			else
				MovePhrase(topPhrase, range, timeDelta, copy, undoCount, undoType);
		}
		phrase = nextPhrase;
	}
}

void _AmDropEntry::MoveLink(	AmPhraseEvent* pe, AmTime timeDelta,
								bool copy, int32* undoCount, AmEvent::EventType* undoType)
{
	ArpASSERT(pe);
	AmLinkEvent*	newLink = new AmLinkEvent(pe->StartTime() + timeDelta, pe->Phrase() );
	if (newLink) {
		mPhrases.push_back(newLink);
		(*undoCount)++;
//		if ( *undoType != AmEvent::_NUM_TYPE + 1) {
//			if (*undoType == AmEvent::_NUM_TYPE) *undoType = event->Type();
//			else if (*undoType != event->Type()) *undoType = (AmEvent::EventType)(AmEvent::_NUM_TYPE + 1);
//		}
	}
	
	if (!copy) mSrcTrack->RemoveEvent(NULL, pe);
}

void _AmDropEntry::MovePhrase(	AmPhraseEvent* topPhrase, AmRange range, AmTime timeDelta,
								bool copy, int32* undoCount, AmEvent::EventType* undoType)
{
	AmNode*			n = topPhrase->Phrase()->FindNode(range.start);
	AmNode*			nextN;
	AmPhraseEvent*	tmpPe = NULL;
	while (n && ( topPhrase->EventRange(n->Event()).start <= range.end) ) {
		nextN = n->next;
		if (!tmpPe) {
			tmpPe = new AmRootPhraseEvent();
			if (tmpPe && tmpPe->Phrase() ) {
				BMessage	props;
				if (topPhrase->Phrase()->GetProperties(props) == B_OK)
					tmpPe->Phrase()->SetProperties(props);
			}
		}
		if (n->Event() && tmpPe && tmpPe->Phrase() ) {
			MoveEvent(topPhrase, tmpPe, n->Event(), timeDelta, copy, undoCount, undoType);
		}
		n = nextN;
	}
	if ( tmpPe && !tmpPe->IsEmpty() ) {
		mPhrases.push_back(tmpPe);
	}
	if ( topPhrase->IsEmpty() ) {
		mSrcTrack->RemoveEvent(NULL, topPhrase);
	}
}

void _AmDropEntry::MoveEvent(	AmPhraseEvent* topPhrase, AmPhraseEvent* newPhrase,
								AmEvent* event, AmTime timeDelta, bool copy,
								int32* undoCount, AmEvent::EventType* undoType)
{
	ArpASSERT(topPhrase && newPhrase && event);
	if (!newPhrase || !event) return;	
	/* Since I might remove the event, always copy it.  This
	 * lets it get safely deleted when the remove undo operation
	 * falls off the stack.
	 */
	AmEvent*		eventCopy = event->Copy();
	if (!copy) mSrcTrack->RemoveEvent(topPhrase->Phrase(), event);
	if (eventCopy) {
		eventCopy->SetStartTime(topPhrase->EventRange(eventCopy).start + timeDelta);
//		newPhrase->SetEventStartTime(eventCopy, topPhrase->EventRange(eventCopy).start + timeDelta);
		newPhrase->Add(eventCopy);
		(*undoCount)++;
		if ( *undoType != AmEvent::_NUM_TYPE + 1) {
			if (*undoType == AmEvent::_NUM_TYPE) *undoType = event->Type();
			else if (*undoType != event->Type()) *undoType = (AmEvent::EventType)(AmEvent::_NUM_TYPE + 1);
		}
	}
}

void _AmDropEntry::AddEvents()
{
	ArpASSERT(mDestTrack);
	if (!mDestTrack) return;
	for (uint32 k = 0; k < mPhrases.size(); k++) {
		mDestTrack->AddEvent( NULL, mPhrases[k] );
	}
	mPhrases.resize(0);
}
