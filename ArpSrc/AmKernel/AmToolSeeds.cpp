/* AmToolSeeds.cpp
 */
#include <cstdio>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmToolKeyHandler.h"
#include "AmKernel/AmToolSeeds.h"
#include "AmKernel/AmToolSeedViews.h"
#include "AmKernel/AmTrack.h"

/* These are all the names used in the configuration methods.  NOTE THAT THESE
 * ARE SHARED WITH AmToolSeedViews.cpp, so if you change them here, you better
 * change them there.
 */
static const char*	FLAGS_STR			= "flags";
//static const char*	TARGET_FLAGS_STR	= "target_flags";

static AmTime new_event_time(float xPixel, const AmToolTarget* target, AmTime quantize)
{
	ArpASSERT(target);
	const AmTimeConverter&	mtc = target->TimeConverter();
	return mtc.PixelToTick(xPixel) - (mtc.PixelToTick(xPixel) % quantize);
}

static const AmTime		QUANT_PRIMES		= 3*5*7;

static inline AmTime quantize(AmTime inTime, AmTime fullTime)
{
	const int64 t = ((int64)inTime)*QUANT_PRIMES;
	return (t-(t%fullTime)) / QUANT_PRIMES;
}

/*************************************************************************
 * AM-TOOL-SEED-I
 *************************************************************************/
AmToolSeedI::~AmToolSeedI()
{
}

status_t AmToolSeedI::GetSeedInfo(	uint32 index,
									BString& outLabel,
									BString& outKey)
{
	if (index == 0) {
		outLabel = "Box";
		outKey = "arp:Box";
		return B_OK;
	} else if (index == 1) {
		outLabel = "Create";
		outKey = "arp:Create";
		return B_OK;
	} else if (index == 2) {
		outLabel = "Curve";
		outKey = "arp:Bezier";
		return B_OK;
	} else if (index == 3) {
		outLabel = "Move";
		outKey = "arp:Move";
		return B_OK;
	} else if (index == 4) {
		outLabel = "Refilter";
		outKey = "arp:Refilter";
		return B_OK;
	} else if (index == 5) {
		outLabel = "Touch";
		outKey = "arp:Touch";
		return B_OK;
	} else if (index == 6) {
		outLabel = "Transform";
		outKey = "arp:Transform";
		return B_OK;
	} else return B_ERROR;
}

status_t AmToolSeedI::GetSeedInfo(	const BString& key,
									BString& outLabel)
{
	if (key == "arp:Box") {
		outLabel = "Box";
		return B_OK;
	} else if (key == "arp:Create") {
		outLabel = "Create";
		return B_OK;
	} else if (key == "arp:Bezier") {
		outLabel = "Curve";
		return B_OK;
	} else if (key == "arp:Move") {
		outLabel = "Move";
		return B_OK;
	} else if (key == "arp:Refilter") {
		outLabel = "Refilter";
		return B_OK;
	} else if (key == "arp:Touch") {
		outLabel = "Touch";
		return B_OK;
	} else if (key == "arp:Transform") {
		outLabel = "Transform";
		return B_OK;
	} else return B_ERROR;
}

AmToolSeedI* AmToolSeedI::NewSeed(const BString& key)
{
	if (key.Compare("arp:Box") == 0) return new AmBoxToolSeed();
	else if (key.Compare("arp:Create") == 0) return new AmCreateToolSeed();
	else if (key.Compare("arp:Bezier") == 0) return new AmBezierToolSeed();
	else if (key.Compare("arp:Move") == 0) return new AmMoveToolSeed();
	else if (key.Compare("arp:Refilter") == 0) return new AmRefilterToolSeed();
	else if (key.Compare("arp:Touch") == 0) return new AmTouchToolSeed();
	else if (key.Compare("arp:Transform") == 0) return new AmTransformToolSeed();
	else return NULL;
}

BView* AmToolSeedI::NewView(const BString& key)
{
	if (key == "arp:Box") return new AmBoxSeedView();
	else if (key == "arp:Create") return new AmCreateSeedView();
	else if (key == "arp:Bezier") return new AmBezierSeedView();
	else if (key == "arp:Move") return new AmMoveSeedView();
	else if (key == "arp:Refilter") return new AmRefilterSeedView();
	else if (key == "arp:Touch") return new AmTouchSeedView();
	else if (key == "arp:Transform") return new AmTransformSeedView();
	else return NULL;
}

status_t AmToolSeedI::ConfigureView(BView* view,
									AmToolRef toolRef,
									const BString& factoryKey,
									const BString& viewKey,
									const BString& seedKey)
{
	AmToolSeedView*		v = dynamic_cast<AmToolSeedView*>(view);
	if (!v) return B_ERROR;
	v->SetToolRef(toolRef);
	v->SetInfo(factoryKey, viewKey, seedKey);
	return B_OK;
}

uint32 AmToolSeedI::Flags() const
{
	return mFlags;
}

AmToolKeyHandler* AmToolSeedI::NewToolKeyHandler() const
{
	return 0;
}

void AmToolSeedI::KeyDown(AmSongRef songRef, AmToolTarget* target, char byte)
{
}

void AmToolSeedI::PostMouseDown(	AmSongRef songRef,
									AmToolTarget* target,
									AmSelectionsI* selections,
									BPoint where)
{
}

void AmToolSeedI::PostMouseMoved(	AmSongRef songRef,
									AmToolTarget* target,
									AmSelectionsI* selections,
									BPoint where)
{
}

void AmToolSeedI::PostMouseUp(	AmSongRef songRef,
								AmToolTarget* target,
								AmSelectionsI* selections,
								BPoint where)
{
}

bool AmToolSeedI::NeedsProcessHack() const
{
	return true;
}

bool static is_first_tempo(AmPhrase* container, AmEvent* event)
{
	ArpASSERT(event && container);
	if (!container) return false;
	if (event->Type() != event->TEMPOCHANGE_TYPE) return false;
	AmNode*		head = container->HeadNode();
	if (!head) return false;
	return head->Event() == event;
}

void AmToolSeedI::ProcessEvent(	AmTrack* track, AmPhraseEvent* topPhrase,
								AmEvent* event, uint32 flags)
{
	ArpASSERT(track && topPhrase && event);
	if (event->IsDeleted() ) {
		if ( !is_first_tempo(topPhrase->Phrase(), event) ) {
			/* Always note that the event is changing in some way first --
			 * this is important in case one tool changes an event, and the
			 * next deletes it.
			 */
			ChangeEvent(track, topPhrase, event, flags);
			DeleteEvent(track, topPhrase, event, flags);
		}
		return;
	}
	bool		didSomething = false;
	if (topPhrase->Parent() == NULL) {
		AddPhrase(track, topPhrase, flags);
		didSomething = true;
	}
	if (event->Parent() == NULL) {
		AddEvent(track, topPhrase, event, flags);
		didSomething = true;
	}
	/* If I did no other processing, then at least note that I
	 * changed something.
	 */
	if (!didSomething) ChangeEvent(track, topPhrase, event, flags);
}

status_t AmToolSeedI::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	if (!config) return B_ERROR;
	int32		i;
	if (config->FindInt32(FLAGS_STR, &i) == B_OK) mFlags = (uint32)i;
	return B_OK;
}

status_t AmToolSeedI::GetConfiguration(BMessage* config) const
{
	ArpASSERT(config);
	status_t		err;
	if ((err = config->AddInt32(FLAGS_STR, mFlags)) != B_OK) return err;
	return B_OK;
}

void AmToolSeedI::AddPhrase(AmTrack* track, AmPhraseEvent* pe, uint32 flags)
{
	ArpASSERT(track && pe);
	track->AddEvent(NULL, pe);
}

void AmToolSeedI::AddEvent(	AmTrack* track, AmPhraseEvent* topPhrase,
							AmEvent* event, uint32 flags)
{
	ArpASSERT(track && topPhrase && event);
	track->AddEvent(topPhrase->Phrase(), event);
}

void AmToolSeedI::DeleteEvent(	AmTrack* track, AmPhraseEvent* topPhrase,
								AmEvent* event, uint32 flags)
{
	ArpASSERT(track && topPhrase && event);
	if (track->RemoveEvent(topPhrase->Phrase(), event) != B_OK) return;
	/* Remove the container if it's now empty.
	 */
	if (topPhrase->IsEmpty()) {
		track->RemoveEvent(NULL, topPhrase);
	}
}

void AmToolSeedI::ChangeEvent(	AmTrack* track,
								AmPhraseEvent* topPhrase,
								AmEvent* event, uint32 flags)
{
}

// #pragma mark -

/*************************************************************************
 * AM-BOX-TOOL-SEED
 *************************************************************************/
AmBoxToolSeed::AmBoxToolSeed()
{
	mFlags = 0;
}

AmBoxToolSeed::~AmBoxToolSeed()
{
	int32		count = mChanges.CountItems();
	for (int32 k = 0; k < count; k++) {
		AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
		delete undo;
	}
	mChanges.MakeEmpty();
}

AmSelectionsI* AmBoxToolSeed::MouseDown(AmSongRef songRef,
										AmToolTarget* target,
										BPoint where)
{
	mOrigin = mCorner = where;
	AmSelectionsI*	selections = NULL;
	int32		count = mChanges.CountItems();
	for (int32 k = 0; k < count; k++) {
		AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
		delete undo;
	}
	mChanges.MakeEmpty();
	// WRITE TRACK BLOCK
	AmSong*		song = songRef.WriteLock();
	if (song) selections = MouseDown(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	return selections;
}

AmSelectionsI* AmBoxToolSeed::MouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where,
											uint32 code)
{
	ArpASSERT( target && target->View() );
	if (mMoveSeed.IsReady() ) {
		// WRITE TRACK BLOCK
		AmSong*		song = songRef.WriteLock();
		if (song) mMoveSeed.Move(song, target, where);
		songRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
		return NULL;
	}

	mCorner = where;
	if (target && target->View() ) target->View()->Invalidate();
	return NULL;
}

AmSelectionsI* AmBoxToolSeed::MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song) {
		if (mMoveSeed.IsReady() ) mMoveSeed.Finished(song);
		else selections = MouseUp(song, target, where);
	}
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	if (target && target->View() ) target->View()->Invalidate();
	return selections;	
}

void AmBoxToolSeed::PostMouseUp(AmSongRef songRef,
								AmToolTarget* target,
								AmSelectionsI* selections,
								BPoint where)
{
	/* Get rid of any events held by the undo that haven't
	 * actually changed, then add all the ones that have changes to the song.
	 */
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song && song->UndoContext() ) {
		int32		count = mChanges.CountItems();
		for (int32 k = 0; k < count; k++) {
			AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
			if (undo) {
				undo->ScrubUnchanged();
				if (undo->HasChanges() ) {
					song->UndoContext()->AddOperation(undo, BResEditor::B_ANY_UNDO_MERGE);
				} else delete undo;
			}
		}
	}
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	mChanges.MakeEmpty();
}

void AmBoxToolSeed::DrawOn(BView* view, BRect clip)
{
	ArpASSERT(view);
	BRect		r(	std::min(mOrigin.x, mCorner.x), std::min(mOrigin.y, mCorner.y),
					std::max(mOrigin.x, mCorner.x), std::max(mOrigin.y, mCorner.y) );
	if (r.left < clip.left) r.left = clip.left;
	if (r.top < clip.top) r.top = clip.top;
	if (r.right > clip.right) r.right = clip.right;
	if (r.bottom > clip.bottom) r.bottom = clip.bottom;

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->SetHighColor(255, 255, 0, 127);
	view->FillRect(r);
	view->SetDrawingMode(mode);
}

AmToolSeedI* AmBoxToolSeed::Copy() const
{
	return new AmBoxToolSeed();
}

AmSelectionsI* AmBoxToolSeed::MouseDown(AmSong* song, AmToolTarget* target,
										BPoint where)
{
	ArpASSERT(song && target);
	/* Find any events the mouse was clicked on.  If we have any, answer them.
	 */
	bool			foundEvent = false;
	track_id		tid;
	AmSelectionsI*	selections = NULL;
	AmTime			quantize = target->TrackWinProperties().GridTime();
	for (uint32 ti = 0; (tid = target->TrackWinProperties().OrderedTrackAt(ti).TrackId()) != 0; ti++) {
		AmTrack*	track = song->Track(tid);
		if (track) {
			AmPhraseEvent*	topPhrase;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
			if (!event) event = target->EventAt(track, new_event_time(where.x, target, quantize), where.y, &topPhrase, &extraData);
			if (event) {
				/* If the first event I click on is included in the selections, then
				 * return those selections for moving.
				 */
				if (!selections) {
					selections = target->TrackWinProperties().Selections();
					if (selections && selections->IncludesEvent(tid, topPhrase, event) ) {
						mMoveSeed.Prepare(song, target, selections, where, quantize);
						if (target->Flags()&target->DRAG_TIME_ONLY) mMoveSeed.SetDragTimeOnly(true);
						return selections;
					}
				}
				/* Add the event to the selections -- either the existing selections if
				 * SHIFT is held down, otherwise a new selection.
				 */
				if (!selections || !(modifiers()&B_SHIFT_KEY) ) {
					selections = AmSelectionsI::NewSelections();
				}
				if (!selections) return NULL;
				if (!selections->IncludesEvent(tid, topPhrase, event) )
					selections->AddEvent(tid, topPhrase, event, extraData);
				foundEvent = true;
			}
		}
	}
	if (foundEvent) {
		mMoveSeed.Prepare(song, target, selections, where, quantize);
		if (target->Flags()&target->DRAG_TIME_ONLY) mMoveSeed.SetDragTimeOnly(true);
		return selections;
	} else return NULL;
}

AmSelectionsI* AmBoxToolSeed::MouseUp(	AmSong* song, AmToolTarget* target,
										BPoint where)
{
	ArpASSERT(target);
	AmTrackWinPropertiesI&	props = target->TrackWinProperties();
	AmSelectionsI*			selections;

	( (modifiers() & B_SHIFT_KEY) )
			? selections = props.Selections()
			: selections = NULL;
	if (!selections) selections = AmSelectionsI::NewSelections();
	if (!selections) return NULL;

	AmTrackRef			trackRef;
	for (uint32 k = 0; (trackRef = props.OrderedTrackAt(k)).IsValid(); k++) {
		const AmTrack*	track = song->Track(trackRef.TrackId() );
		if (track) GetEvents(target, *track, BRect(mOrigin, where), selections);
	}

	/* I need to prepare these guys, in case they get processed by a pipeline.
	 */
	uint32				trackCount = selections->CountTracks();
	for (uint32 k = 0; k < trackCount; k++) {
		track_id			tid;
		AmPhraseEvent*		topPhrase;
		AmEvent*			event;
		AmChangeEventUndo*	undo = NULL;
		for (uint32 j = 0; selections->EventAt(k, j, &tid, &topPhrase, &event) == B_OK; j++) {
			if (!undo) undo = UndoForTrack(song, tid, true);
			if (undo) undo->EventChanging(topPhrase->Phrase(), event);
		}
	}

	return selections;
}

void AmBoxToolSeed::GetEvents(	AmToolTarget* target,
								const AmTrack& track,
								BRect rect,
								AmSelectionsI* selections) const
{
	ArpASSERT(selections);
	const AmTimeConverter&	mtc = target->TimeConverter();
	/* Convert the pixel rect into MIDI data coordinates. */
	AmTime			start = mtc.PixelToTick(rect.left),
					end = mtc.PixelToTick(rect.right);
	int32			top = target->MoveYValueFromPixel(rect.top),
					bottom = target->MoveYValueFromPixel(rect.bottom);
	/* Swap if start or bottom is less than end or top. */
	if (end < start) {
		AmTime		swap = start;
		start = end;
		end = swap;
	}
	if (bottom < top) {
		int32 		swap = top;
		top = bottom;
		bottom = swap;
	}
	/* Loop over all the phrase events in the track's Phrases. */
	const AmPhrase&	phrases = track.Phrases();
	AmNode*			n = phrases.HeadNode();
	AmPhraseEvent*	topPhrase;
	while (n && n->StartTime() <= end) {
		if ( (topPhrase = dynamic_cast<AmPhraseEvent*>( n->Event() ))
				&& topPhrase->EndTime() >= start) {
			GetTopEvents(	target,
							track.Id(),
							topPhrase,
							start, top, end, bottom,
							selections);
		}
		n = n->next;
	}
}

void AmBoxToolSeed::GetTopEvents(	AmToolTarget* target,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections) const
{
	ArpASSERT(topPhrase && topPhrase->Phrase());
	if (!topPhrase || !topPhrase->Phrase() ) return;
	AmNode*		n = topPhrase->Phrase()->HeadNode();
	AmRange		eventRange;
	AmEvent*	event = NULL;
	while (n && (event = n->Event())
			&& (eventRange = topPhrase->EventRange(event)).start <= end) {
		if ( target->IsInteresting(event) ) {
			if ( target->EventIntersects(event, eventRange, start, top, end, bottom)
					&& ( !(selections->IncludesEvent(trackId, topPhrase, event)) ) ) {
				selections->AddEvent(trackId, topPhrase, event);
			}
		} else if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe) GetEvents(target, trackId, topPhrase, pe, start, top, end, bottom, selections);
		}
		n = n->next;
	}
}

void AmBoxToolSeed::GetEvents(	AmToolTarget* target,
								track_id trackId,
								AmPhraseEvent* topPhrase,
								AmPhraseEvent* phrase,
								AmTime start, int32 top, AmTime end, int32 bottom,
								AmSelectionsI* selections) const
{
	ArpASSERT(topPhrase && topPhrase->Phrase() && phrase && phrase->Phrase());
	if (!topPhrase || !topPhrase->Phrase() ) return;
	if (!phrase || !phrase->Phrase() ) return;
	AmNode*		n = phrase->Phrase()->HeadNode();
	AmRange		eventRange;
	AmEvent*	event = NULL;
	/* This is a little different than events at the top level -- if any
	 * of the events in this phrase are found to be within the selection range,
	 * then add THE PHRASE to the selections.
	 */
	while (n && (event = n->Event())
			&& (eventRange = topPhrase->EventRange(event)).start <= end) {
		if ( target->IsInteresting(event) ) {
			if ( target->EventIntersects(event, eventRange, start, top, end, bottom)
					&& ( !(selections->IncludesEvent(trackId, topPhrase, phrase)) ) ) {
				selections->AddEvent(trackId, topPhrase, phrase);
			}
		} else if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe) GetEvents(target, trackId, topPhrase, pe, start, top, end, bottom, selections);
		}
		n = n->next;
	}
}

void AmBoxToolSeed::ChangeEvent(AmTrack* track,
								AmPhraseEvent* topPhrase,
								AmEvent* event,
								uint32 flags)
{
	ArpASSERT(track && event);
	if (flags&NO_PROCESSING_FLAG) return;
	AmChangeEventUndo*	undo = UndoForTrack(NULL, track->Id(), false);
	if (!undo) return;
	undo->EventChanged(event);
}

AmChangeEventUndo* AmBoxToolSeed::UndoForTrack(AmSong* song, track_id tid, bool create)
{
	int32		count = mChanges.CountItems();
	for (int32 k = 0; k < count; k++) {
		AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
		if (undo->TrackId() == tid) return undo;
	}
	if (!create) return NULL;
	if (!song) return NULL;
	AmTrack*	t = song->Track(tid);
	if (!t) return NULL;
	AmChangeEventUndo*		undo = new AmChangeEventUndo(t);
	if (!undo) return NULL;
	mChanges.AddItem(undo);
	return undo;
}

// #pragma mark -

/*************************************************************************
 * AM-BEZIER-TOOL-SEED
 *************************************************************************/
const char* AM_BEZIER_SEED_PT1START		= "pt1s";
const char* AM_BEZIER_SEED_PT1END		= "pt1e";
const char* AM_BEZIER_SEED_PT2START		= "pt2s";
const char* AM_BEZIER_SEED_PT2END		= "pt2e";
const char* AM_BEZIER_SEED_FRAME		= "frame";
const char* AM_BEZIER_SEED_MODE			= "mode";

AmBezierToolSeed::AmBezierToolSeed()
		: mMode(TRANFORM_MODE), mFrame(0), mFrameStep(0.1),
		  mPt1Frame0(0, 0), mPt1Frame1(0, 0),
		  mPt2Frame0(1, 1), mPt2Frame1(1, 1)
{
	mFlags = 0;
}

AmBezierToolSeed::AmBezierToolSeed(const AmBezierToolSeed& o)
		: mMode(o.mMode), mFrame(o.mFrame), mFrameStep(o.mFrameStep),
		  mPt1Frame0(o.mPt1Frame0), mPt1Frame1(o.mPt1Frame1),
		  mPt2Frame0(o.mPt2Frame0), mPt2Frame1(o.mPt2Frame1)
{
	mFlags = o.mFlags;
}

AmBezierToolSeed::~AmBezierToolSeed()
{
	int32		count = mChanges.CountItems();
	for (int32 k = 0; k < count; k++) {
		AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
		delete undo;
	}
	mChanges.MakeEmpty();
}

class _BezierToolKeyHandler : public AmToolKeyHandler
{
public:
	_BezierToolKeyHandler()		{ }

	bool CanHandle(char byte) const
	{
		if (byte == 'z' || byte == 'x') return true;
		return false;
	}
};

AmToolKeyHandler* AmBezierToolSeed::NewToolKeyHandler() const
{
	return new _BezierToolKeyHandler();
}

void AmBezierToolSeed::KeyDown(	AmSongRef songRef,
								AmToolTarget* target, char byte)
{
	ArpASSERT(target && target->View());

	if (byte == 'z') mFrame -= mFrameStep;
	else if (byte == 'x') mFrame += mFrameStep;
	
	if (mFrame < 0) mFrame = 0;
	else if (mFrame > 1) mFrame = 1;
	
	SetInteriorPoints();
	if (target && target->View()) target->View()->Invalidate();
}

AmSelectionsI* AmBezierToolSeed::MouseDown(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	mOrigin = mPt1 = mPt2 = mCorner = where;
	return NULL;
}

AmSelectionsI* AmBezierToolSeed::MouseMoved(AmSongRef songRef,
											AmToolTarget* target,
											BPoint where,
											uint32 code)
{
	ArpASSERT(target && target->View());
	mCorner = where;
	SetInteriorPoints();

	float		stepMin = 0.001, stepMax = 0.1, wMax = 1000;
	float		w = fabs(mCorner.x - mOrigin.x);
	mFrameStep = fabs(w - wMax) / 10000;
	if (mFrameStep < stepMin) mFrameStep = stepMin;
	else if (mFrameStep > stepMax) mFrameStep = stepMax;
	
	if (target && target->View()) target->View()->Invalidate();
	return NULL;
}

AmSelectionsI* AmBezierToolSeed::MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	mCorner = where;
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song) selections = MouseUp(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	if (target && target->View() ) target->View()->Invalidate();
	return selections;	
}

void AmBezierToolSeed::PostMouseUp(AmSongRef songRef,
								AmToolTarget* target,
								AmSelectionsI* selections,
								BPoint where)
{
	/* Get rid of any events held by the undo that haven't
	 * actually changed, then add all the ones that have changes to the song.
	 */
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song && song->UndoContext() ) {
		int32		count = mChanges.CountItems();
		for (int32 k = 0; k < count; k++) {
			AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
			if (undo) {
				undo->ScrubUnchanged();
				if (undo->HasChanges() ) {
					song->UndoContext()->AddOperation(undo, BResEditor::B_ANY_UNDO_MERGE);
				} else delete undo;
			}
		}
	}
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	mChanges.MakeEmpty();
}

void AmBezierToolSeed::DrawOn(BView* view, BRect clip)
{
	ArpASSERT(view);
	DrawBezierOn(view, clip);
}

AmToolSeedI* AmBezierToolSeed::Copy() const
{
	return new AmBezierToolSeed(*this);
}

status_t AmBezierToolSeed::SetConfiguration(const BMessage* config)
{
	status_t	err = inherited::SetConfiguration(config);
	if (err != B_OK) return err;
	BPoint		pt;
	if (config->FindPoint(AM_BEZIER_SEED_PT1START, &pt) == B_OK) mPt1Frame0 = pt;
	if (config->FindPoint(AM_BEZIER_SEED_PT1END, &pt) == B_OK) mPt1Frame1 = pt;
	if (config->FindPoint(AM_BEZIER_SEED_PT2START, &pt) == B_OK) mPt2Frame0 = pt;
	if (config->FindPoint(AM_BEZIER_SEED_PT2END, &pt) == B_OK) mPt2Frame1 = pt;
	float		f;
	if (config->FindFloat(AM_BEZIER_SEED_FRAME, &f) == B_OK) {
		mFrame = f;
		if (mFrame < 0) mFrame = 0;
		else if (mFrame > 1) mFrame = 1;
	}
	int32		i32;
	if (config->FindInt32(AM_BEZIER_SEED_MODE, &i32) == B_OK)
		mMode = uint32(i32);
	return B_OK;
}

status_t AmBezierToolSeed::GetConfiguration(BMessage* config) const
{
	status_t	err = inherited::GetConfiguration(config);
	if (err != B_OK) return err;
	if ((err = config->AddPoint(AM_BEZIER_SEED_PT1START, mPt1Frame0)) != B_OK) return err;
	if ((err = config->AddPoint(AM_BEZIER_SEED_PT1END, mPt1Frame1)) != B_OK) return err;
	if ((err = config->AddPoint(AM_BEZIER_SEED_PT2START, mPt2Frame0)) != B_OK) return err;
	if ((err = config->AddPoint(AM_BEZIER_SEED_PT2END, mPt2Frame1)) != B_OK) return err;
	if ((err = config->AddFloat(AM_BEZIER_SEED_FRAME, mFrame)) != B_OK) return err;
	if ((err = config->AddInt32(AM_BEZIER_SEED_MODE, mMode)) != B_OK) return err;
	return B_OK;
}

void AmBezierToolSeed::DrawStraightOn(BView* view, BRect clip)
{
	drawing_mode	mode = view->DrawingMode();
	float			penSize = view->PenSize();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetPenSize(3);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->SetHighColor(255, 255, 0, 127);
	view->StrokeLine(mOrigin, mCorner);
	view->SetDrawingMode(mode);
	view->SetPenSize(penSize);
}

static inline void create_ray(BPoint& bezPoint, const BPoint& a, const BPoint& b, float step)
{
  bezPoint.x = a.x + (b.x - a.x)*step;
  bezPoint.y = a.y + (b.y - a.y)*step;
}

static void bezier(	BPoint& bezPoint, const BPoint& pt0, const BPoint& pt1,
					const BPoint& pt2, const BPoint& pt3, float step)
{
  BPoint		ab, bc, cd, abbc, bccd;
  create_ray(ab, pt0, pt1, step);			//  a to b 
  create_ray(bc, pt1, pt2, step);			//  b to c 
  create_ray(cd, pt2, pt3, step);			//  c to d
  create_ray(abbc, ab, bc, step);			//  ab to bc
  create_ray(bccd, bc, cd, step);			//  bc to cd
  create_ray(bezPoint, abbc, bccd, step);	//  main point on the curve
}

void AmBezierToolSeed::DrawBezierOn(BView* view, BRect clip)
{
	drawing_mode	mode = view->DrawingMode();
	float			penSize = view->PenSize();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetPenSize(3);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->SetHighColor(255, 255, 0, 127);

	BPoint		curr;
	BPoint		from = mOrigin;
	float		aw = fabs(mCorner.x - mOrigin.x);
	for (float i = 0; i < aw; i++) {
		bezier(curr, mOrigin, mPt1, mPt2, mCorner, i / aw);
		view->StrokeLine(from, curr);
//		printf("%ld %ld\n", int32(curr.x), int32(curr.y));
		from = curr;
	}

	view->SetDrawingMode(mode);
	view->SetPenSize(penSize);
}

AmSelectionsI* AmBezierToolSeed::MouseUp(	AmSong* song, AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(target);
	AmTrackWinPropertiesI&	props = target->TrackWinProperties();
	AmSelectionsI*			selections;

	( (modifiers() & B_SHIFT_KEY) )
			? selections = props.Selections()
			: selections = NULL;
	if (!selections) selections = AmSelectionsI::NewSelections();
	if (!selections) return NULL;

	AmTrackRef			trackRef;
	am_trans_params		params;
	params.flags = target->TRANSFORM_Y;
	for (uint32 k = 0; (trackRef = props.OrderedTrackAt(k)).IsValid(); k++) {
		AmTrack*		track = song->Track(trackRef.TrackId() );
		if (track) {
			AmChangeEventUndo*		undo = new AmChangeEventUndo(track);
			GetEvents(target, *track, BRect(mOrigin, where), selections, undo, params);
			if (undo && undo->CountEvents() > 0)
				mChanges.AddItem(undo);
		}
	}
	/* If this is the create mode, all the GetEvents() call did was delete all
	 * events within range.  Now I need to create new ones.
	 */
	AmTime			lastTime = -1;
	int32			m, d;
	AmTime			v;
	props.GetSplitGridTime(&m, &v, &d);
	if (mMode&CREATE_MODE) {
		BPoint			curr;
		BPoint			from = mOrigin;
		float			aw = fabs(mCorner.x - mOrigin.x);
		for (float i = 0; i < aw; i++) {
			bezier(curr, mOrigin, mPt1, mPt2, mCorner, i / aw);
			AmTime		startTime = target->TimeConverter().PixelToTick(curr.x);
			AmTime		quant = quantize(startTime, m*((v*2*QUANT_PRIMES)/d));
			if (lastTime < 0 || lastTime != quant) {
				CreateEvents(props, song, target, startTime, curr.y, selections);
				lastTime = quant;
			}
			from = curr;
		}
	}

	return selections;
}

void AmBezierToolSeed::CreateEvents(AmTrackWinPropertiesI& props, AmSong* song, AmToolTarget* target,
									AmTime time, float y, AmSelectionsI* selections)
{
	ArpASSERT(song && selections);
	AmTrackRef			trackRef;
	for (uint32 k = 0; (trackRef = props.OrderedTrackAt(k)).IsValid(); k++) {
		AmTrack*		track = song->Track(trackRef.TrackId() );
		AmEvent*		e = 0;
		if (track && (e = target->NewEvent(*track, time, y)) != 0) {
			AmPhraseEvent*	pe = target->PhraseEventNear(track, e->StartTime() );
			bool			created = false;
			if (!pe) {
				pe = new AmRootPhraseEvent();
				created = true;
			}
			if (!pe) e->Delete();
			else {
				if (created) AddPhrase(track, pe, 0);
				AddEvent(track, pe, e, 0);
				selections->AddEvent(track->Id(), pe, e);
			}
		}
	}
}

void AmBezierToolSeed::GetEvents(	AmToolTarget* target,
									AmTrack& track,
									BRect rect,
									AmSelectionsI* selections,
									AmChangeEventUndo* undo,
									const am_trans_params& params)
{
	ArpASSERT(selections);
	const AmTimeConverter&	mtc = target->TimeConverter();
	/* Convert the pixel rect into MIDI data coordinates. */
	AmTime			start = mtc.PixelToTick(rect.left),
					end = mtc.PixelToTick(rect.right);
	int32			top = target->MoveYValueFromPixel(rect.top),
					bottom = target->MoveYValueFromPixel(rect.bottom);
	/* Swap if start or bottom is less than end or top. */
	if (end < start) {
		AmTime		swap = start;
		start = end;
		end = swap;
	}
	if (bottom < top) {
		int32 		swap = top;
		top = bottom;
		bottom = swap;
	}
	/* Loop over all the phrase events in the track's Phrases. */
	const AmPhrase&	phrases = track.Phrases();
	AmNode*			n = phrases.HeadNode();
	AmPhraseEvent*	topPhrase;
	while (n && n->StartTime() <= end) {
		AmNode*		next = n->next;
		if ( (topPhrase = dynamic_cast<AmPhraseEvent*>( n->Event() ))
				&& topPhrase->EndTime() >= start) {
			GetTopEvents(	target, track,
							track.Id(),
							topPhrase,
							start, top, end, bottom,
							selections, undo, params);
		}
		n = next;
	}
}

static inline bool event_intersects(const AmRange& range, AmTime start, AmTime end)
{
	if (range.start > end || range.end < start) return false;
	return true;
}

void AmBezierToolSeed::GetTopEvents(AmToolTarget* target,
									AmTrack& track,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections,
									AmChangeEventUndo* undo,
									const am_trans_params& params)
{
	ArpASSERT(topPhrase && topPhrase->Phrase());
	if (!topPhrase || !topPhrase->Phrase() ) return;
	AmNode*		n = topPhrase->Phrase()->HeadNode();
	AmRange		eventRange;
	AmEvent*	event = NULL;

	float		grid = fabs(mCorner.x - mOrigin.x);
	AmNode*		next;
	
	while (n && (event = n->Event())
			&& (eventRange = topPhrase->EventRange(event)).start <= end) {
		next = n->next;
		if ( target->IsInteresting(event) ) {
// Always use the full Y bounds of the view, this seems more appropriate in general and
// is DEFINITELY more appropriate in specific places (like the tempo change view)
			if (event_intersects(eventRange, start, end)
//			if ( target->EventIntersects(event, eventRange, start, top, end, bottom)
					&& ( !(selections->IncludesEvent(trackId, topPhrase, event)) ) ) {
				/* If in create mode, delete all the events I would normally transform.
				 * I'll run through and create the events later.
				 */
				if (mMode&CREATE_MODE) {
					DeleteEvent(&track, topPhrase, event, 0);
				} else {
					selections->AddEvent(trackId, topPhrase, event);
					if (undo) undo->EventChanging(topPhrase->Phrase(), event);

					float		x = target->TimeConverter().TickToPixel(event->StartTime());
					BPoint		bez;
					float		step = (x >= mOrigin.x) ? (x - mOrigin.x) : (mOrigin.x - x);
					bezier(bez, mOrigin, mPt1, mPt2, mCorner, step / grid);
					// If move
					if (mMode&MOVE_MODE) {
						AmTime		oldTime;
						int32		oldY, newY = target->MoveYValueFromPixel(bez.y);
						target->GetMoveValues(*topPhrase, event, &oldTime, &oldY);
						target->SetMove(*topPhrase, event, oldTime, oldY, 0, newY - oldY, target->TRANSFORM_Y);
					} else {
						target->SetTransform(track, *topPhrase, event, BPoint(0, bez.y), params);
					}
				}
			}
		} else if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe) GetEvents(target, track, trackId, topPhrase, pe, start, top, end, bottom, selections, undo, params);
		}
		n = next;
	}
}

void AmBezierToolSeed::GetEvents(	AmToolTarget* target,
									AmTrack& track,
									track_id trackId,
									AmPhraseEvent* topPhrase,
									AmPhraseEvent* phrase,
									AmTime start, int32 top, AmTime end, int32 bottom,
									AmSelectionsI* selections,
									AmChangeEventUndo* undo,
									const am_trans_params& params)
{
	ArpASSERT(topPhrase && topPhrase->Phrase() && phrase && phrase->Phrase());
	if (!topPhrase || !topPhrase->Phrase() ) return;
	if (!phrase || !phrase->Phrase() ) return;
	AmNode*		n = phrase->Phrase()->HeadNode();
	AmRange		eventRange;
	AmEvent*	event = NULL;
	/* This is a little different than events at the top level -- if any
	 * of the events in this phrase are found to be within the selection range,
	 * then add THE PHRASE to the selections.
	 */
	while (n && (event = n->Event())
			&& (eventRange = topPhrase->EventRange(event)).start <= end) {
		if ( target->IsInteresting(event) ) {
			if ( target->EventIntersects(event, eventRange, start, top, end, bottom)
					&& ( !(selections->IncludesEvent(trackId, topPhrase, phrase)) ) ) {
				selections->AddEvent(trackId, topPhrase, phrase);
				if (undo) undo->EventChanging(topPhrase->Phrase(), phrase);
			}
		} else if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe) GetEvents(target, track, trackId, topPhrase, pe, start, top, end, bottom, selections, undo, params);
		}
		n = n->next;
	}
}

void AmBezierToolSeed::ChangeEvent(	AmTrack* track,
									AmPhraseEvent* topPhrase,
									AmEvent* event,
									uint32 flags)
{
	ArpASSERT(track && event);
	if (flags&NO_PROCESSING_FLAG) return;
	AmChangeEventUndo*	undo = UndoForTrack(NULL, track->Id());
	if (!undo) return;
	undo->EventChanged(event);
}

AmChangeEventUndo* AmBezierToolSeed::UndoForTrack(AmSong* song, track_id tid)
{
	int32		count = mChanges.CountItems();
	for (int32 k = 0; k < count; k++) {
		AmChangeEventUndo*	undo = (AmChangeEventUndo*)mChanges.ItemAt(k);
		if (undo->TrackId() == tid) return undo;
	}
	return NULL;
}

inline double linear_interp(const double d1, const double d2, const double weight)
{
	return d2*weight+d1*(1.0-weight);
}

void AmBezierToolSeed::SetInteriorPoints()
{
	float		w = mCorner.x - mOrigin.x, h = mCorner.y - mOrigin.y;
	mPt1.x = mOrigin.x + (w * linear_interp(mPt1Frame0.x, mPt1Frame1.x, mFrame));
	mPt1.y = mOrigin.y + (h * linear_interp(mPt1Frame0.y, mPt1Frame1.y, mFrame));
	mPt2.x = mOrigin.x + (w * linear_interp(mPt2Frame0.x, mPt2Frame1.x, mFrame));
	mPt2.y = mOrigin.y + (h * linear_interp(mPt2Frame0.y, mPt2Frame1.y, mFrame));
}

// #pragma mark -

/**********************************************************************
 * _AM-MOVE-ENTRY
 **********************************************************************/
_AmMoveEntry::_AmMoveEntry()
		: mEvent(NULL), mContainer(NULL), mStart(-1), mY(0)
{
}

_AmMoveEntry::_AmMoveEntry(	AmEvent* event,
							AmPhraseEvent* container,
							AmToolTarget* target)
		: mEvent(event), mContainer(container)
{
	ArpASSERT(event && container);
	target->GetMoveValues(*container, event, &mStart, &mY);
}

_AmMoveEntry::_AmMoveEntry(const _AmMoveEntry& o)
		: mEvent(o.mEvent), mContainer(o.mContainer),
		  mStart(o.mStart), mY(o.mY)
{
}

_AmMoveEntry& _AmMoveEntry::operator=(const _AmMoveEntry& o)
{
	mEvent = o.mEvent;
	mContainer = o.mContainer;
	mStart = o.mStart;
	mY = o.mY;
	return *this;
}

// #pragma mark -

_AmTrackMoveEntry::_AmTrackMoveEntry()
		: mTrackId(0), mUndoCache(NULL)
{
}

_AmTrackMoveEntry::_AmTrackMoveEntry(	AmSong* song,
										AmToolTarget* target,
										uint32 trackIndex,
										AmSelectionsI* selections)
		: mTrackId(0), mUndoCache(NULL)
{
	AmTrack*		track = NULL;
	track_id		tid;
	AmPhraseEvent*	topPhrase;
	AmEvent*		event;
	for (uint32 ei = 0; selections->EventAt(trackIndex, ei, &tid, &topPhrase, &event) == B_OK; ei++) {
		if (!track) {
			track = song->Track(tid);
			if (track) {
				mTrackId = track->Id();
				mUndoCache = new AmChangeEventUndo(track);
			}
		}
		mEntries.push_back(_AmMoveEntry(event, topPhrase, target) );
		if (mUndoCache) mUndoCache->EventChanging(topPhrase->Phrase(), event);
	}
}

_AmTrackMoveEntry::_AmTrackMoveEntry(const _AmTrackMoveEntry& o)
{
	mTrackId = o.mTrackId;
	mUndoCache = o.mUndoCache;
	mEntries.resize(0);
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back(o.mEntries[k]);
}

_AmTrackMoveEntry::~_AmTrackMoveEntry()
{
//	ArpASSERT(mUndoCache == NULL);
}

_AmTrackMoveEntry& _AmTrackMoveEntry::operator=(const _AmTrackMoveEntry& o)
{
	mTrackId = o.mTrackId;
	mUndoCache = o.mUndoCache;
	mEntries.resize(0);
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back(o.mEntries[k]);
	return *this;
}

bool _AmTrackMoveEntry::IsEmpty() const
{
	return mTrackId == 0 && mEntries.size() < 1;
}

bool _AmTrackMoveEntry::Move(	AmSong* song,
								AmToolTarget* target,
								AmTime timeDelta, int32 yDelta,
								bool change)
{
	AmTrack*		track = song->Track(mTrackId);
	if (!track) return change;
	/* Make a copy before each event is moved purely to see if there are
	 * any changes made to the event.  Whenever a change is made to one
	 * or more events, I'll want to perform the phrase.
	 */
	AmEvent*		copy = NULL;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (!change) copy = mEntries[k].mEvent->Copy();
		if (copy) copy->RemoveEvent();
		MoveEvent(track, target, &(mEntries[k]), timeDelta, yDelta);
		if (copy && !(copy->Equals( mEntries[k].mEvent )) ) change = true;
		if (copy) copy->Delete();
		copy = NULL;
	}
	return change;
}

void _AmTrackMoveEntry::PostMove(	AmSong* song,
									AmSelectionsI* oldSelections,
									AmSelectionsI* newSelections)
{
	ArpASSERT(newSelections);
	AmTrack*	track = song->Track(mTrackId);
	if (!track) return;
	
//	if (flags&RESTORE_DELETED_EVENTS_FLAG) {
		for (uint32 k = 0; k < mEntries.size(); k++) {
			if (mEntries[k].mEvent->IsDeleted() )
				mEntries[k].mEvent->Undelete();
			newSelections->AddEvent(mTrackId, mEntries[k].mContainer, mEntries[k].mEvent);
		}
//	}
	
	if (oldSelections) {
		AmPhraseEvent*		topPhrase;
		AmEvent*			event;
		int32				extraData;
		for (uint32 k = 0; oldSelections->EventAt(mTrackId, k, &topPhrase, &event, &extraData) == B_OK; k++) {
			if (!newSelections->IncludesEvent(mTrackId, topPhrase, event) )
				track->RemoveEvent(topPhrase->Phrase(), event);
		}
	}
}

void _AmTrackMoveEntry::Finished(AmSong* song, bool moveOccurred)
{
	ArpASSERT(song);
	if (moveOccurred && mUndoCache && song->UndoContext() ) {
		for (uint32 k=0; k<mEntries.size(); k++ ) {
			mUndoCache->EventChanged(mEntries[k].mEvent);
		}
		song->UndoContext()->AddOperation(mUndoCache, BResEditor::B_ANY_UNDO_MERGE);
		mUndoCache = NULL;
	}
	mEntries.resize(0);
	delete mUndoCache;
	mUndoCache = NULL;
}

void _AmTrackMoveEntry::MoveEvent(	AmTrack* track,
									AmToolTarget* target,
									_AmMoveEntry* entry,
									AmTime timeDelta,
									int32 yDelta)
{
	ArpASSERT(entry && entry->mContainer && entry->mEvent);

	AmTime		newTime = entry->mStart + timeDelta;
	if (newTime < 0) newTime = 0;

	AmEvent*		event = entry->mEvent;
	AmPhraseEvent*	container = entry->mContainer;
	if (!container->Phrase() ) return;
	
	if (target->IsInteresting(event) ) {
		target->SetMove(*container, event, entry->mStart, entry->mY, timeDelta, yDelta, 0);
	} else {
		container->SetEventStartTime(event, newTime);
	}
}

// #pragma mark -

/*************************************************************************
 * AM-MOVE-TOOL-SEED
 *************************************************************************/
AmMoveToolSeed::AmMoveToolSeed()
		: mGridCache(PPQN), mMoveOccurred(false), mDragTimeOnly(false)
{
	mFlags = 0;	
}

AmMoveToolSeed::AmMoveToolSeed(const AmMoveToolSeed& o)
		: mGridCache(PPQN), mMoveOccurred(false), mDragTimeOnly(false)
{
	mFlags = o.mFlags;
}

AmMoveToolSeed::~AmMoveToolSeed()
{
}

AmSelectionsI* AmMoveToolSeed::MouseDown(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song) selections = MouseDown(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	target->TrackWinProperties().SetSelections(selections);
	return NULL;
}

AmSelectionsI* AmMoveToolSeed::MouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where,
											uint32 code)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	if (IsReady() ) {
		// WRITE TRACK BLOCK
		AmSong*		song = songRef.WriteLock();
		if (song) selections = Move(song, target, where);
		songRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
	}
	return selections;	
}

AmSelectionsI* AmMoveToolSeed::MouseUp(	AmSongRef songRef,
										AmToolTarget* target,
										BPoint where)
{
	ArpASSERT(target);
	// WRITE SONG BLOCK
	AmSong*		song = songRef.WriteLock();
	Finished(song);
	songRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
	return NULL;
}

AmToolSeedI* AmMoveToolSeed::Copy() const
{
	return new AmMoveToolSeed(*this);
}

void AmMoveToolSeed::Prepare(	AmSong* song,
								AmToolTarget* target,
								AmSelectionsI* selections,
								BPoint origin,
								AmTime gridTime)
{
	ArpASSERT(song && target && selections);
	mGridCache = gridTime;
	mEntries.resize(0);
	uint32			trackCount = selections->CountTracks();
	for (uint32 ti = 0; ti < trackCount; ti++) {
		mEntries.push_back(_AmTrackMoveEntry(song, target, ti, selections) );
	}
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].IsEmpty() ) {
			mEntries.erase(mEntries.begin() + k);
			k = 0;
		}
	}
	mOrigin = origin;
	const AmTimeConverter&	mtc = target->TimeConverter();
	mTimeOrigin = mtc.PixelToTick(origin.x);
	mTimeOrigin = mTimeOrigin - (mTimeOrigin % mGridCache);
	if (mTimeOrigin < 0) mTimeOrigin = 0;
	mMoveOccurred = false;
}

bool AmMoveToolSeed::IsReady()
{
	return mEntries.size() > 0;
}
AmSelectionsI* AmMoveToolSeed::Move(AmSong* song,
									AmToolTarget* target,
									BPoint where)
{
	ArpASSERT(IsReady() && song && target);
	const AmTimeConverter&	mtc = target->TimeConverter();

	AmTime		timeDelta;
	int32		yDelta;
	target->GetMoveDelta(mOrigin, where, &timeDelta, &yDelta);

	AmTime		quantizedWhere = mtc.PixelToTick(where.x);
	quantizedWhere = quantizedWhere - (quantizedWhere % mGridCache);
	timeDelta = quantizedWhere - mTimeOrigin;
	
	return Move(song, target, timeDelta, yDelta);
}

AmSelectionsI* AmMoveToolSeed::Move(AmSong* song,
									AmToolTarget* target,
									AmTime timeDelta, int32 yDelta)
{
	ArpASSERT(IsReady() && target && song);
	
	AmTime			oldSongEnd = song->CountEndTime();
	bool			change = false;
	if (mDragTimeOnly) yDelta = 0;
	for (uint32 k = 0; k < mEntries.size(); k++ ) {
		change = mEntries[k].Move(song, target, timeDelta, yDelta, change);
	}
	AmSelectionsI*	selections = NULL;
	if (change) {
		selections = PostMove(song, target->TrackWinProperties().Selections() );
		mMoveOccurred = true;
		target->Perform(song, selections);
	}
	
	AmTime		newSongEnd = song->CountEndTime();
	if (oldSongEnd != newSongEnd) song->EndTimeChangeNotice(newSongEnd);
	return selections;
}

AmSelectionsI* AmMoveToolSeed::PostMove(AmSong* song, AmSelectionsI* oldSelections)
{
	ArpASSERT(song);
	AmSelectionsI*		newSelections = AmSelectionsI::NewSelections();
	if (!newSelections) return NULL;

	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].PostMove(song, oldSelections, newSelections);
	
	return newSelections;
}

void AmMoveToolSeed::Finished(AmSong* song)
{
	ArpASSERT(song);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		mEntries[k].Finished(song, mMoveOccurred);
	}
	mEntries.resize(0);
	mMoveOccurred = false;
}

void AmMoveToolSeed::SetDragTimeOnly(bool dragTimeOnly)
{
	mDragTimeOnly = dragTimeOnly;
}

AmSelectionsI* AmMoveToolSeed::MouseDown(	AmSong* song,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(song && target);
	mGridCache = target->TrackWinProperties().GridTime();
	/* If the mouse was clicked on an existing event, then prepare it to be
	 * moved and do nothing more.  If I can't find an event exactly where the
	 * user checked, look to see if there's one at my quantized position.
	 */
	track_id		tid;
	AmSelectionsI*	selections = target->TrackWinProperties().Selections();
	for (uint32 ti = 0; (tid = selections->TrackAt(ti)) != 0; ti++) {
		AmTrack*	track = song->Track(tid);
		if (track) {
			AmPhraseEvent*	topPhrase;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
			if (!event) event = target->EventAt(track, new_event_time(where.x, target, mGridCache), where.y, &topPhrase, &extraData);
			if (event) {
				/* If there are previous selections and the user is holding down the
				 * modifier key, add the new event to the previous selections.
				 */
				if (selections && (modifiers() & B_COMMAND_KEY) ) {
					selections->AddEvent(tid, topPhrase, event, extraData);
				/* If there are no current selections, or this item is not
				 * already selected, select this event.
				 */
				} else if (!selections || !(selections->IncludesEvent(tid, topPhrase, event)) ) {
					if (!(selections = AmSelectionsI::NewSelections()) ) return NULL;
					selections->AddEvent(tid, topPhrase, event, extraData);
				/* Finally, we have a selections and this event is included.  Make sure
				 * the entry contains the extraData I just found up above.  This allows users
				 * to click on different parts of the even to change different properties, for
				 * targets that support that behaviour.
				 */
				} else {
					ArpASSERT(selections);
					selections->SetExtraData(tid, topPhrase, event, extraData);
				}
			}
		}
	}
	/* Now that I've set up the selections, I want to prepare them for moving.
	 */
	Prepare(song, target, selections, where, mGridCache);
	return selections;
}

// #pragma mark -

/*************************************************************************
 * AM-TOUCH-TOOL-SEED
 *************************************************************************/
AmTouchToolSeed::AmTouchToolSeed()
{
	mFlags = 0;
}

AmTouchToolSeed::~AmTouchToolSeed()
{
}

AmSelectionsI* AmTouchToolSeed::MouseDown(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	// READ TRACK BLOCK
	const AmSong*			song = songRef.ReadLock();
	if (song) selections = FindSelections(song, target, where);
	songRef.ReadUnlock(song);
	// END READ TRACK BLOCK
	return selections;	
}

AmSelectionsI* AmTouchToolSeed::MouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where,
											uint32 code)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	// READ TRACK BLOCK
	const AmSong*			song = songRef.ReadLock();
	if (song) selections = FindSelections(song, target, where);
	songRef.ReadUnlock(song);
	// END READ TRACK BLOCK
	return selections;	
}

AmSelectionsI* AmTouchToolSeed::MouseUp(AmSongRef songRef,
										AmToolTarget* target,
										BPoint where)
{
	return NULL;
}

AmToolSeedI* AmTouchToolSeed::Copy() const
{
	return new AmTouchToolSeed();
}

status_t AmTouchToolSeed::SetConfiguration(const BMessage* config)
{
	return inherited::SetConfiguration(config);
}

status_t AmTouchToolSeed::GetConfiguration(BMessage* config) const
{
	return inherited::GetConfiguration(config);
}

AmSelectionsI* AmTouchToolSeed::FindSelections(	const AmSong* song,
												AmToolTarget* target,
												BPoint where)
{
	ArpASSERT(song && target);
	AmSelectionsI*	selections = NULL;
	AmTrackRef		trackRef;
	for (uint32 k = 0; (trackRef = target->TrackWinProperties().OrderedTrackAt(k)).IsValid(); k++) {
		const AmTrack*	track = song->Track(trackRef.TrackId() );
		if (track) {
			AmPhraseEvent*	topPhrase;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
			if (event) {
				if (!selections) selections = AmSelectionsI::NewSelections();
				if (selections) selections->AddEvent(track->Id(), topPhrase, event, extraData);
			}
		}
	}
	return selections;
}

// #pragma mark -

/**********************************************************************
 * _AM-TRANS-ENTRY
 **********************************************************************/
_AmTransEntry::_AmTransEntry()
		: mTopPhrase(NULL), mEvent(NULL)
{
}

_AmTransEntry::_AmTransEntry(	AmPhraseEvent* topPhrase,
								AmEvent* event,
								AmToolTarget* target,
								int32 extraData)
		: mTopPhrase(topPhrase), mEvent(event)
{
	mParams.extra_data = extraData;
	target->GetOriginalTransform(event, mParams);
}

_AmTransEntry::_AmTransEntry(const _AmTransEntry& o)
{
	mTopPhrase = o.mTopPhrase;
	mEvent = o.mEvent;
	mParams = o.mParams;
}

_AmTransEntry& _AmTransEntry::operator=(const _AmTransEntry& o)
{
	mTopPhrase = o.mTopPhrase;
	mEvent = o.mEvent;
	mParams = o.mParams;
	return *this;
}

// #pragma mark -

_AmTrackTransEntry::_AmTrackTransEntry()
		: mTrackId(0), mUndo(NULL)
{
}

_AmTrackTransEntry::_AmTrackTransEntry(	AmSong* song,
										AmToolTarget* target,
										uint32 trackIndex,
										AmSelectionsI* selections)
		: mTrackId(0), mUndo(NULL)
{
	AmTrack*		track = NULL;
	track_id		tid;
	AmPhraseEvent*	topPhrase;
	AmEvent*		event;
	int32			extraData;
	for (uint32 ei = 0; selections->EventAt(trackIndex, ei, &tid, &topPhrase, &event, &extraData) == B_OK; ei++) {
		if (!track) {
			track = song->Track(tid);
			if (track) {
				mTrackId = track->Id();
				mUndo = new AmChangeEventUndo(track);
			}
		}
		if (track) PrepareEvent(topPhrase, event, target, extraData);
	}
}

_AmTrackTransEntry::_AmTrackTransEntry(	AmTrack* track, AmPhraseEvent* container,
										AmEvent* event, int32 extraData,
										AmToolTarget* target, BPoint where)
		: mTrackId(track->Id() ), mUndo(NULL)
{
	TransformOneByOneEvent(track, container, event, extraData, target, where);
}

_AmTrackTransEntry::_AmTrackTransEntry(const _AmTrackTransEntry& o)
{
	mTrackId = o.mTrackId;
	mUndo = o.mUndo;
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back(o.mEntries[k]);
}

_AmTrackTransEntry& _AmTrackTransEntry::operator=(const _AmTrackTransEntry& o)
{
	mTrackId = o.mTrackId;
	mUndo = o.mUndo;
	mEntries.resize(0);
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back(o.mEntries[k]);
	return *this;
}

bool _AmTrackTransEntry::IsEmpty() const
{
	return mTrackId == 0 && mEntries.size() < 1;
}

bool _AmTrackTransEntry::Transform(	AmSong* song,
									AmToolTarget* target,
									int32 xDelta, int32 yDelta,
									uint32 targetFlags,
									bool change)
{
	ArpASSERT(song && target);
	AmTrack*		track = song->Track(mTrackId);
	if (!track) return change;
	/* Make a copy before each event is moved purely to see if there are
	 * any changes made to the event.  Whenever a change is made to one
	 * or more events, I'll want to perform the phrase.
	 */
	AmEvent*	copy = NULL;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (!change) copy = mEntries[k].mEvent->Copy();
		if (copy) copy->RemoveEvent();
		mEntries[k].mParams.delta_x = xDelta;
		mEntries[k].mParams.delta_y = yDelta;
		uint32		flags =	TransformEvent(track, target, &(mEntries[k]), targetFlags);
		if (copy && !(copy->Equals(mEntries[k].mEvent)) ) {
			if (!(flags&AM_TRANS_NO_PLAY)) change = true;
		}
		if (copy) copy->Delete();
		copy = NULL;
	}
	return change;
}

void _AmTrackTransEntry::TransformOneByOneEvent(AmTrack* track, AmPhraseEvent* container, AmEvent* event,
												int32 extraData, AmToolTarget* target, BPoint where)
{
	ArpASSERT(track && target && container);

	if (!mUndo) mUndo = new AmChangeEventUndo(track);
	if (mUndo) mUndo->EventChanging(container->Phrase(), event);
	if (container->Phrase() ) {
		am_trans_params		params;
		params.extra_data = extraData;
		params.flags = target->TRANSFORM_Y;
		target->SetTransform(*track, *container, event, where, params);
	}
	if (mUndo) mUndo->EventChanged(event);
}

void _AmTrackTransEntry::Finished(AmSong* song)
{
	if (mUndo) {
		for (uint32 k = 0; k < mEntries.size(); k++) {
			mUndo->EventChanged(mEntries[k].mEvent);
		}
		if (song && song->UndoContext() )
			song->UndoContext()->AddOperation(mUndo, BResEditor::B_ANY_UNDO_MERGE);
		else
			delete mUndo;
		mUndo = NULL;
	}
	mEntries.resize(0);
}

void _AmTrackTransEntry::PrepareEvent(	AmPhraseEvent* topPhrase,
										AmEvent* event,
										AmToolTarget* target,
										int32 extraData)
{
	if (target->IsInteresting(event) || event->Type() != event->PHRASE_TYPE) {
		mEntries.push_back( _AmTransEntry(topPhrase, event, target, extraData) );
		if (mUndo) mUndo->EventChanging(topPhrase->Phrase(), event);
	} else if (event->Type() == event->PHRASE_TYPE) {
		AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(event);
		if (pe) {
			AmNode*		n = pe->Phrase()->HeadNode();
			while (n) {
				/* Ignore the phrase events.  This is because the selection
				 * object currently stores all phrase events in any phrase events,
				 * so I don't want to add those twice.
				 */
				if (n->Event()->Type() != n->Event()->PHRASE_TYPE)
					PrepareEvent(topPhrase, n->Event(), target, extraData);
				n = n->next;
			}
		}
	}
}

uint32 _AmTrackTransEntry::TransformEvent(	AmTrack* track,
											AmToolTarget* target,
											_AmTransEntry* entry,
											uint32 targetFlags)
{
	ArpASSERT(track && entry && entry->mTopPhrase);
	uint32		flags = 0;
	if (entry->mTopPhrase->Phrase() ) {
		entry->mParams.flags = targetFlags;
		flags = target->SetTransform(*track, *entry->mTopPhrase, entry->mEvent, entry->mParams);
	}
	return flags;
}

// #pragma mark -

/*************************************************************************
 * AM-TRANSFORM-TOOL-SEED
 *************************************************************************/
#if 0
static void print_trans_events(std::vector<_AmTrackTransEntry>& trackEntries)
{
	printf("Transform entry events:\n");
	for (uint32 k = 0; k < trackEntries.size(); k++) {
		for (uint32 k2 = 0; k2 < trackEntries[k].mEntries.size(); k2++) {
			trackEntries[k].mEntries[k2].mEvent->Print();
		}
	}
}
#endif

AmTransformToolSeed::AmTransformToolSeed()
		: mTargetFlags(0), mGridCache(PPQN)
{
	mFlags = EN_MASSE_FLAG;	
}

AmTransformToolSeed::AmTransformToolSeed(const AmTransformToolSeed& o)
		: mTargetFlags(o.mTargetFlags), mGridCache(PPQN)
{
	mFlags = o.mFlags;
}

AmTransformToolSeed::~AmTransformToolSeed()
{
}

AmSelectionsI* AmTransformToolSeed::MouseDown(	AmSongRef songRef,
												AmToolTarget* target,
												BPoint where)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song) selections = MouseDown(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	return selections;	
}

AmSelectionsI* AmTransformToolSeed::MouseMoved(	AmSongRef songRef,
												AmToolTarget* target,
												BPoint where,
												uint32 code)
{
	ArpASSERT(target);
	if (IsReady() && mFlags&EN_MASSE_FLAG) {
		// WRITE TRACK BLOCK
		AmSong*		song = songRef.WriteLock();
		if (song) Transform(song, target, where);
		songRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
	} else if (mFlags&ONE_BY_ONE_FLAG) {
		// WRITE TRACK BLOCK
		AmSong*		song = songRef.WriteLock();
		if (song) TransformOneByOneEvent(song, target, where);
		songRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
	}
	return target->TrackWinProperties().Selections();
}

AmSelectionsI* AmTransformToolSeed::MouseUp(AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	// WRITE SONG BLOCK
	AmSong*		song = songRef.WriteLock();
	Finished(song);
	songRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
	return NULL;
}

AmToolSeedI* AmTransformToolSeed::Copy() const
{
	return new AmTransformToolSeed(*this);
}

status_t AmTransformToolSeed::SetConfiguration(const BMessage* config)
{
	status_t	err = inherited::SetConfiguration(config);
	if (err != B_OK) return err;
//	int32		i;
//	if (config->FindInt32(TARGET_FLAGS_STR, &i) == B_OK) mTargetFlags = (uint32)i;
	return B_OK;
}

status_t AmTransformToolSeed::GetConfiguration(BMessage* config) const
{
	status_t	err = inherited::GetConfiguration(config);
	if (err != B_OK) return err;
//	if ((err = config->AddInt32(TARGET_FLAGS_STR, mTargetFlags)) != B_OK) return err;
	return B_OK;
}

void AmTransformToolSeed::Prepare(	AmSong* song,
									AmToolTarget* target,
									AmSelectionsI* selections,
									BPoint origin,
									AmTime gridTime)
{
	ArpASSERT(song && target && selections);
	mGridCache = gridTime;
	mEntries.resize(0);
	mTargetFlags = target->TRANSFORM_BOTH;
	uint32			trackCount = selections->CountTracks();
	for (uint32 ti = 0; ti < trackCount; ti++) {
		mEntries.push_back(_AmTrackTransEntry(song, target, ti, selections) );
	}
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].IsEmpty() ) {
			mEntries.erase(mEntries.begin() + k);
			k = 0;
		}
	}
	mOrigin = origin;
	mAxisChangeOrigin = origin;
	mLastPoint = origin;
}

bool AmTransformToolSeed::IsReady()
{
	return mEntries.size() > 0;
}
void AmTransformToolSeed::Transform(AmSong* song,
									AmToolTarget* target,
									BPoint where)
{
	ArpASSERT(IsReady() && target);
	SetAxisFlag(target, where);
	am_trans_params		params;
	target->GetDeltaTransform(mOrigin, where, params);
	Transform(song, target, params.delta_x, params.delta_y);
}

void AmTransformToolSeed::Transform(AmSong* song,
									AmToolTarget* target,
									int32 xDelta, int32 yDelta)
{
	ArpASSERT(IsReady() && target);
	
	AmTime		oldSongEnd = song->CountEndTime();

	bool		change = false;
	for (uint32 k = 0; k < mEntries.size(); k++)
		change = mEntries[k].Transform(song, target, xDelta, yDelta, mTargetFlags, change);
	if (change) target->Perform(song, target->TrackWinProperties().Selections() );
//if (change) printf("There's a change\n");
//else printf("No change\n");
	AmTime		newSongEnd = song->CountEndTime();
	if (oldSongEnd != newSongEnd) song->EndTimeChangeNotice(newSongEnd);
}

void AmTransformToolSeed::Finished(AmSong* song)
{
	ArpASSERT(song);
	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].Finished(song);
	mEntries.resize(0);
	mTargetFlags = AmToolTarget::TRANSFORM_BOTH;

}

AmSelectionsI* AmTransformToolSeed::MouseDown(	AmSong* song,
												AmToolTarget* target,
												BPoint where)
{
	ArpASSERT(song && target);
	/* If I'm setup to simply transform events as I come across them,
	 * then I don't need to mess with the selections.
	 */
	if (mFlags&ONE_BY_ONE_FLAG) return NULL;

	mGridCache = target->TrackWinProperties().GridTime();
	/* If the mouse was clicked on an existing event, then prepare it to be
	 * moved and do nothing more.  If I can't find an event exactly where the
	 * user checked, look to see if there's one at my quantized position.
	 */
	track_id		tid;
	AmSelectionsI*	selections = target->TrackWinProperties().Selections();
	for (uint32 ti = 0; (tid = target->TrackWinProperties().OrderedTrackAt(ti).TrackId()) != 0; ti++) {
		AmTrack*	track = song->Track(tid);
		if (track) {
			AmPhraseEvent*	topPhrase;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
			if (!event) event = target->EventAt(track, new_event_time(where.x, target, mGridCache), where.y, &topPhrase, &extraData);
			if (event) {
				/* If there are previous selections and the user is holding down the
				 * modifier key, add the new event to the previous selections.
				 */
				if (selections && (modifiers() & B_SHIFT_KEY) ) {
					if (!selections->IncludesEvent(tid, topPhrase, event))
						selections->AddEvent(tid, topPhrase, event, extraData);
				/* If there are no current selections, or this item is not
				 * already selected, select this event.
				 */
				} else if (!selections || !(selections->IncludesEvent(tid, topPhrase, event)) ) {
					if (!(selections = AmSelectionsI::NewSelections()) ) return NULL;
					selections->AddEvent(tid, topPhrase, event, extraData);
				/* Finally, we have a selections and this event is included.  Make sure
				 * the entry contains the extraData I just found up above.  This allows users
				 * to click on different parts of the even to change different properties, for
				 * targets that support that behaviour.
				 */
				} else {
					ArpASSERT(selections);
					selections->SetExtraData(tid, topPhrase, event, extraData);
				}
			}
		}
	}
	if (!selections) return NULL;
	/* Now that I've set up the selections, I want to prepare them for transforming.
	 */
	Prepare(song, target, selections, where, mGridCache);
	return selections;
}

void AmTransformToolSeed::SetAxisFlag(const AmToolTarget* target, BPoint where)
{
	float	tolerance = 4;

	/* First do the check that occurs once a flag has already initially
	 * been set.
	 * If the user has moved further along the non-currently flagged axis,
	 * then switch to that axis.
	 */
	if (mTargetFlags != target->TRANSFORM_BOTH) {
		/* If the current motion is aligned with the current axis, then
		 * reset the counter that keeps track of the tolerance.
		 */
		float		deltaX = fabs(where.x - mLastPoint.x);
		float		deltaY = fabs(where.y - mLastPoint.y);
		if( ( deltaX >= deltaY && mTargetFlags == target->TRANSFORM_X )
				|| ( deltaY >= deltaX && mTargetFlags == target->TRANSFORM_Y ) ) {
			mAxisChangeOrigin = where;
		/* Otherwise, the user is traveling along the wrong axis.  If they've
		 * gone beyond the tolerance, flip the axis.
		 */
		} else {
			deltaX = fabs(where.x - mAxisChangeOrigin.x);
			deltaY = fabs(where.y - mAxisChangeOrigin.y);
			if (mTargetFlags == target->TRANSFORM_X && deltaY > tolerance) {
				mTargetFlags = target->TRANSFORM_Y;
			} else if (mTargetFlags == target->TRANSFORM_Y && deltaX > tolerance) {
				mTargetFlags = target->TRANSFORM_X;
			}
		}
	}

	/* If no flag has yet been set, but the user has moved enough,
	 * set the axis s/he is moving on.
	 */
	if (mTargetFlags == target->TRANSFORM_BOTH) {
		float	change = fabs(where.x - mOrigin.x);
		if (change > tolerance) {
			mTargetFlags = target->TRANSFORM_X;
		} else {
			change = fabs(where.y - mOrigin.y);
			if( change > tolerance ) mTargetFlags = target->TRANSFORM_Y;
		}
	}

	mLastPoint = where;
}

void AmTransformToolSeed::TransformOneByOneEvent(AmSong* song, AmToolTarget* target, BPoint where)
{
	AmTrackRef		trackRef;
	for (uint32 k = 0; (trackRef = target->TrackWinProperties().OrderedTrackAt(k)).IsValid(); k++) {
		AmTrack*	track = song->Track(trackRef.TrackId() );		
		if (track) {
			AmPhraseEvent*	container;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &container, &extraData);
			if (event) {
				bool		foundEntry = false;
				for (uint32 te = 0; te < mEntries.size(); te++) {
					if (mEntries[te].mTrackId == track->Id() ) {
						mEntries[te].TransformOneByOneEvent(track, container, event, extraData, target, where);
						foundEntry = true;
					}
				}
				if (!foundEntry) mEntries.push_back(_AmTrackTransEntry(track, container, event, extraData, target, where));
			}
		}
	}
}

// #pragma mark -

/*************************************************************************
 * AM-CREATE-TOOL-SEED
 *************************************************************************/
AmCreateToolSeed::AmCreateToolSeed()
		: mForceMoveHack(false), mGridCache(PPQN)
{
	mFlags = MOVE_FLAG;
}

AmCreateToolSeed::AmCreateToolSeed(const AmCreateToolSeed& o)
		: mGridCache(PPQN)
{
	mFlags = o.mFlags;
}

AmCreateToolSeed::~AmCreateToolSeed()
{
}

AmSelectionsI* AmCreateToolSeed::MouseDown(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(target);
	mCreateEntries.resize(0);
	AmSelectionsI*		selections = NULL;
	mForceMoveHack = false;
	// WRITE TRACK BLOCK
	AmSong*		song = songRef.WriteLock();
	if (song) selections = MouseDown(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	return selections;
}

AmSelectionsI* AmCreateToolSeed::MouseMoved(AmSongRef songRef,
											AmToolTarget* target,
											BPoint where,
											uint32 code)
{
	ArpASSERT(target);
	AmSelectionsI*		selections = NULL;
	// WRITE TRACK BLOCK
	AmSong*		song = songRef.WriteLock();
	if (song) selections = MouseMoved(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	return selections;
}

AmSelectionsI* AmCreateToolSeed::MouseUp(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(target);
	// WRITE SONG BLOCK
	AmSong*		song = songRef.WriteLock();
	if (mMoveSeed.IsReady() ) mMoveSeed.Finished(song);
	if (mTransformSeed.IsReady() ) mTransformSeed.Finished(song);
	if (mRefilterSeed.IsReady() ) mRefilterSeed.Finished(song);
	songRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
	mCreateEntries.resize(0);
	return NULL;
}

void AmCreateToolSeed::PostMouseDown(	AmSongRef songRef,
										AmToolTarget* target,
										AmSelectionsI* selections,
										BPoint where)
{
	ArpASSERT(target);
	if (selections) {
		// WRITE SONG BLOCK
		AmSong*		song = songRef.WriteLock();
		if (song) {
			target->Perform(song, selections);
			if (mForceMoveHack || mFlags&MOVE_FLAG) {
				mMoveSeed.Prepare(song, target, selections, where, mGridCache);
				if (target->Flags()&target->DRAG_TIME_ONLY) mMoveSeed.SetDragTimeOnly(true);
			}
		}
		songRef.WriteUnlock(song);
		// END WRITE SONG BLOCK
	}
	mForceMoveHack = false;
}

void AmCreateToolSeed::PostMouseMoved(	AmSongRef songRef,
										AmToolTarget* target,
										AmSelectionsI* selections,
										BPoint where)
{
	if (selections && target && mRefilterSeed.IsReady()) {
		// READ TRACK BLOCK
		const AmSong*	song = songRef.ReadLock();
		if (song) target->Perform(song, selections);
		songRef.ReadUnlock(song);
		// END READ TRACK BLOCK
	}
}

bool AmCreateToolSeed::NeedsProcessHack() const
{
	if (mForceMoveHack) return false;
	else return true;
}

AmToolSeedI* AmCreateToolSeed::Copy() const
{
	return new AmCreateToolSeed(*this);
}

AmSelectionsI* AmCreateToolSeed::MouseDown(	AmSong* song,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(song && target);
	mGridCache = target->TrackWinProperties().GridTime();
	/* If the mouse was clicked on an existing event, then prepare it to be
	 * moved and do nothing more.  If I can't find an event exactly where the
	 * user checked, look to see if there's one at my quantized position,
	 * which is where I'll be placing a new event -- I don't want to put a new
	 * one on top of the old one.
	 */
	AmSelectionsI*	selections = MouseDownOnEvent(song, target, where);
	if (selections) {
		mForceMoveHack = true;
		return selections;
	}
	
	AmTrack*		track = song->Track(target->TrackWinProperties().OrderedTrackAt(0) );
	if (!track) return NULL;
	/* If there was no event where the mouse was clicked then prepare
	 * a new event according to the rules of the view.
	 */
	selections = CreateEvents(song, target, where);
	if (mFlags&ADDN_FLAG) {
//		if (selections) target->Perform(song, selections);
	} else if (mFlags&TRANSFORM_FLAG) {
//		if (selections) target->Perform(song, selections);
		if (selections) mTransformSeed.Prepare(song, target, selections, where, mGridCache);
	} else if (mFlags&REFILTER_FLAG) {
//		if (selections) target->Perform(song, selections);
//		if (selections) mRefilterSeed.Prepare(song, target, selections, where, mGridCache);
		if (selections) {
			mRefilterSeed.mEntries.resize(0);
			uint32			trackCount = selections->CountTracks();
			for (uint32 ti = 0; ti < trackCount; ti++) {
				track_id	tid = selections->TrackAt(ti);
				if (tid != 0) mRefilterSeed.mEntries.push_back(_AmTrackRefilterEntry(song, tid));
			}
		}
	} else {
		// Preparing the move seed is now handled in the PostMouseDown().
//		if (selections) target->Perform(song, selections);
//		if (selections) mMoveSeed.Prepare(song, target, selections, where, mGridCache);
	}

	if (target->View() ) target->View()->MakeFocus(true);
	return selections;
}

AmSelectionsI* AmCreateToolSeed::MouseMoved(AmSong* song,
											AmToolTarget* target,
											BPoint where)
{
	if (mMoveSeed.IsReady() ) {
		mMoveSeed.Move(song, target, where);
		return NULL;
	}
	if (mFlags&TRANSFORM_FLAG && mTransformSeed.IsReady() ) {
		mTransformSeed.Transform(song, target, where);
		return NULL;
	}
	if (mFlags&REFILTER_FLAG) {
		if (mRefilterSeed.mFlags&mRefilterSeed.RESTORE_FLAG) {
			for (uint32 k = 0; k < mRefilterSeed.mEntries.size(); k++)
				mRefilterSeed.mEntries[k].UndoEverything(song);
		}
		return NewCreatedEvents();
	}

	AmTrack*			track = song->Track(target->TrackWinProperties().OrderedTrackAt(0));
	if (!track) return NULL;
	if (mFlags&ADDN_FLAG) {
		AmPhraseEvent*	pe;
		int32			extraData;
		AmEvent*		event = target->EventAt(track, where, &pe, &extraData);
		if (!event) event = target->EventAt(track, new_event_time(where.x, target, mGridCache), where.y, &pe, &extraData);
		if (!event) {
			AmSelectionsI*	selections = CreateEvents(song, target, where);
			if (selections) target->Perform(song, selections);
			return selections;
		} else {
			if (pe && pe->Phrase() ) {
				target->MergeChangeEvent(event, *pe);
				AmTime	timeVal;
				int32	origY;
				target->GetMoveValues(*pe, event, &timeVal, &origY);
				int32	absY = target->MoveYValueFromPixel(where.y);
				target->SetMove(*pe, event, pe->EventRange(event).start, origY, 0, absY - origY, 0);
				target->DrawChangeEvent(event, *pe);
				AmSelectionsI*	selections = target->TrackWinProperties().Selections();
				if (selections) target->Perform(song, selections);
				return selections;
			}
		}
	}
	return NULL;
}

void AmCreateToolSeed::AddPhrase(AmTrack* track, AmPhraseEvent* pe, uint32 flags)
{
	if (mFlags&REFILTER_FLAG) mRefilterSeed.AddPhrase(track, pe, flags);
	else inherited::AddPhrase(track, pe, flags);
}

void AmCreateToolSeed::AddEvent(AmTrack* track, AmPhraseEvent* topPhrase,
								AmEvent* event, uint32 flags)
{
	if (mFlags&REFILTER_FLAG) mRefilterSeed.AddEvent(track, topPhrase, event, flags);
	else inherited::AddEvent(track, topPhrase, event, flags);
}

void AmCreateToolSeed::DeleteEvent(	AmTrack* track, AmPhraseEvent* topPhrase,
									AmEvent* event, uint32 flags)
{
	if (mFlags&REFILTER_FLAG) mRefilterSeed.DeleteEvent(track, topPhrase, event, flags);
	else inherited::DeleteEvent(track, topPhrase, event, flags);
}

AmSelectionsI* AmCreateToolSeed::MouseDownOnEvent(	AmSong* song,
													AmToolTarget* target,
													BPoint where)
{
	ArpASSERT(song && target);
	/* Find any events the mouse was clicked on.  If we have any, answer them.
	 */
	bool			foundEvent = false;
	track_id		tid;
	AmSelectionsI*	selections = NULL;
	for (uint32 ti = 0; (tid = target->TrackWinProperties().OrderedTrackAt(ti).TrackId()) != 0; ti++) {
		AmTrack*	track = song->Track(tid);
		if (track) {
			AmPhraseEvent*	topPhrase;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
			if (!event) event = target->EventAt(track, new_event_time(where.x, target, mGridCache), where.y, &topPhrase, &extraData);
			if (event) {
				/* If the first event I click on is included in the selections, then
				 * return those selections for moving.
				 */
				if (!selections) {
					selections = target->TrackWinProperties().Selections();
					if (selections && selections->IncludesEvent(tid, topPhrase, event) )
						return selections;
				}
				/* Add the event to the selections -- either the existing selections if
				 * SHIFT is held down, otherwise a new selection.
				 */
				if (!selections || !(modifiers()&B_SHIFT_KEY) ) {
					selections = AmSelectionsI::NewSelections();
				}
				if (!selections) return NULL;
				if (!selections->IncludesEvent(tid, topPhrase, event) )
					selections->AddEvent(tid, topPhrase, event, extraData);
				foundEvent = true;
			}
		}
	}
	if (foundEvent) return selections;
	else return NULL;
}

// #pragma mark -

_CreateAndFilterEntry::_CreateAndFilterEntry()
		: mTrackId(0), mTopPhrase(NULL), mEvent(NULL)
{
}

_CreateAndFilterEntry::_CreateAndFilterEntry(const _CreateAndFilterEntry& o)
		: mTrackId(o.mTrackId), mTopPhrase(NULL), mEvent(NULL)
{
	if (o.mEvent) {
		mEvent = o.mEvent->Copy();
		if (o.mTopPhrase) {
			mTopPhrase = o.mTopPhrase;
			mTopPhrase->IncRefs();
		}
	}
}

_CreateAndFilterEntry::_CreateAndFilterEntry(	track_id tid, AmPhraseEvent* topPhrase,
												AmEvent* event)
		: mTrackId(tid), mTopPhrase(NULL), mEvent(NULL)
{
	ArpASSERT(event);
	if (topPhrase) {
		mTopPhrase = topPhrase;
		mTopPhrase->IncRefs();
	}
	mEvent = event->Copy();
}

_CreateAndFilterEntry::~_CreateAndFilterEntry()
{
	if (mEvent) mEvent->Delete();
	if (mTopPhrase) mTopPhrase->DecRefs();
}

_CreateAndFilterEntry& _CreateAndFilterEntry::operator=(const _CreateAndFilterEntry& o)
{
	if (mEvent) mEvent->Delete();
	if (mTopPhrase) mTopPhrase->DecRefs();
	mEvent = NULL;
	mTopPhrase = NULL;
	
	mTrackId = o.mTrackId;
	if (o.mEvent) {
		mEvent = o.mEvent->Copy();
		if (o.mTopPhrase) {
			mTopPhrase = o.mTopPhrase;
			mTopPhrase->IncRefs();
		}
	}
	return *this;
}

void _CreateAndFilterEntry::AddToSelections(AmSelectionsI* selections) const
{
	if (!mEvent) return;
	AmPhraseEvent*	pe = NULL;
	AmEvent*		e = NULL;
	if (mTopPhrase) pe = mTopPhrase;
	else pe = new AmRootPhraseEvent();
	e = mEvent->Copy();
	if (pe && e) selections->AddEvent(mTrackId, pe, e);
}

// #pragma mark -

AmSelectionsI* AmCreateToolSeed::CreateEvents(	const AmSong* song,
												AmToolTarget* target,
												BPoint where)
{
	ArpASSERT(song && target);
	if (!song || !target) return NULL;
	AmTrackWinPropertiesI&	props = target->TrackWinProperties();
	AmSelectionsI*			selections = (modifiers() & B_SHIFT_KEY) ? props.Selections() : NULL;
	if (!selections) selections = AmSelectionsI::NewSelections();
	if (!selections) return NULL;

	const AmTrack*			track;
	const AmTimeConverter& 	mtc = target->TimeConverter();
	for (uint32 k = 0; (track = song->Track(props.OrderedTrackAt(k))) != NULL; k++) {
		AmEvent*			e = target->NewEvent(*track, mtc.PixelToTick(where.x), where.y );
		if (e) {
			bool			created = false;
			AmPhraseEvent*	pe = target->PhraseEventNear(track, e->StartTime() );
			if (!pe) {
				pe = new AmRootPhraseEvent();
				created = true;
			}
			if (!pe) e->Delete();
			else {
				int32		m, d;
				AmTime		v;
				props.GetSplitGridTime(&m, &v, &d);
				AmTime		startTime = quantize(e->StartTime(), m*((v*2*QUANT_PRIMES)/d));
				e->SetStartTime(startTime);
				mCreateEntries.push_back(_CreateAndFilterEntry(track->Id(), created ? NULL : pe, e));
				selections->AddEvent(track->Id(), pe, e);
			}
		}
	}
	return selections;
}

AmSelectionsI* AmCreateToolSeed::NewCreatedEvents() const
{
	AmSelectionsI*		sel = AmSelectionsI::NewSelections();
	if (!sel) return NULL;
	for (uint32 k = 0; k < mCreateEntries.size(); k++) mCreateEntries[k].AddToSelections(sel);
	return sel;
}

// #pragma mark -

/***************************************************************************
 * _AM-TRACK-REFILTER-ENTRY
 ***************************************************************************/
_AmTrackRefilterEntry::_AmTrackRefilterEntry()
		: mTrackId(0), mUndo(NULL)
{
}

_AmTrackRefilterEntry::_AmTrackRefilterEntry(	AmSong* song, AmToolTarget* target,
												AmSelectionsI* selections,
												uint32 trackIndex)
		: mTrackId(0), mUndo(NULL)
{
	mTrackId = selections->TrackAt(trackIndex);
	AmTrack*		track = song->Track(mTrackId);
	AmPhraseEvent*	topPhrase;
	AmEvent*		event;
	mUndo = track ? new AmChangeEventUndo(track) : NULL;
	track_id		tid;
	for (uint32 k = 0; (selections->EventAt(trackIndex, k, &tid, &topPhrase, &event)) == B_OK; k++) {
		if (mTrackId == tid) {
			mEntries.push_back( _AmMoveEntry(event, topPhrase, target) );
			if (mUndo) mUndo->EventChanging(topPhrase->Phrase(), event);
		}
	}
}

_AmTrackRefilterEntry::_AmTrackRefilterEntry(AmSong* song, track_id tid)
		: mTrackId(tid), mUndo(NULL)
{
	AmTrack*		track = song->Track(mTrackId);
	mUndo = track ? new AmChangeEventUndo(track) : NULL;
}

_AmTrackRefilterEntry::_AmTrackRefilterEntry(const _AmTrackRefilterEntry& o)
{
	mTrackId = o.mTrackId;
	mUndo = o.mUndo;
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back(o.mEntries[k]);
	for (uint32 k = 0; k < o.mUndoList.size(); k++)
		mUndoList.push_back(o.mUndoList[k]);
}

_AmTrackRefilterEntry& _AmTrackRefilterEntry::operator=(const _AmTrackRefilterEntry& o)
{
	mTrackId = o.mTrackId;
	mUndo = o.mUndo;
	mEntries.resize(0);
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back(o.mEntries[k]);
	for (uint32 k = 0; k < o.mUndoList.size(); k++)
		mUndoList.push_back(o.mUndoList[k]);
	return *this;
}

bool _AmTrackRefilterEntry::IsEmpty() const
{
	return mTrackId == 0 && mEntries.size() < 1 && mUndoList.size() < 1;
}

void _AmTrackRefilterEntry::UndoEverything(AmSong* song)
{
	for (int32 k = (int32)mUndoList.size() - 1; k >= 0; k--) {
		mUndoList[k]->Undo();
		delete mUndoList[k];
	}
	mUndoList.resize(0);
	if (mUndo) {
		for (uint32 k = 0; k < mEntries.size(); k++) {
			mUndo->EventChanged(mEntries[k].mEvent);
		}
		if (mUndo->HasChanges()) {
			mUndo->Undo();
		}
	}
	delete mUndo;
	mUndo = NULL;
	AmTrack*	track = song->Track(mTrackId);
	if (track) {
		mUndo = new AmChangeEventUndo(track);
		if (mUndo) {
			for (uint32 k = 0; k < mEntries.size(); k++) {
				mUndo->EventChanging(mEntries[k].mContainer->Phrase(), mEntries[k].mEvent);
			}
		}
	}
}

void _AmTrackRefilterEntry::PostMove(AmSong* song, AmSelectionsI* newSelections)
{
	ArpASSERT(song && newSelections);

	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mEvent->IsDeleted() )
			mEntries[k].mEvent->Undelete();
		newSelections->AddEvent(mTrackId, mEntries[k].mContainer, mEntries[k].mEvent);
	}
}

void _AmTrackRefilterEntry::Finished(AmSong* song)
{
	if (song && song->UndoContext() ) {
		if (mUndo) {
			for (uint32 k = 0; k < mEntries.size(); k++)
				mUndo->EventChanged(mEntries[k].mEvent);
			song->UndoContext()->AddOperation(mUndo, BResEditor::B_ANY_UNDO_MERGE);
		}
		for (uint32 k = 0; k < mUndoList.size(); k++)
			song->UndoContext()->AddOperation(mUndoList[k], BResEditor::B_ANY_UNDO_MERGE);
		mUndoList.resize(0);
	} else {
		delete mUndo;
		for (uint32 k = 0; k < mUndoList.size(); k++) delete mUndoList[k];
		mUndoList.resize(0);
	}
	mUndo = NULL;
	mEntries.resize(0);
}

void _AmTrackRefilterEntry::AddPhrase(AmTrack* track, AmPhraseEvent* pe)
{
	ArpASSERT(track && pe && track->Id() == mTrackId);
	if (track->AddEvent(NULL, pe, NULL, NULL) != B_OK) return;
	AmTrackEventUndo*	undo = new AmTrackEventUndo(track, true, NULL, pe);
	if (undo) mUndoList.push_back(undo);
}

void _AmTrackRefilterEntry::AddEvent(AmTrack* track, AmPhraseEvent* topPhrase, AmEvent* event)
{
	ArpASSERT(track && topPhrase && event && track->Id() == mTrackId);
	if (track->AddEvent(topPhrase->Phrase(), event, NULL, NULL) != B_OK) return;
	AmTrackEventUndo*	undo = new AmTrackEventUndo(track, true, topPhrase->Phrase(), event);
	if (undo) mUndoList.push_back(undo);
}

void _AmTrackRefilterEntry::DeleteEvent(AmTrack* track, AmPhraseEvent* topPhrase, AmEvent* event)
{
	ArpASSERT(track && topPhrase && event && track->Id() == mTrackId);
	if (track->RemoveEvent(topPhrase->Phrase(), event, NULL, NULL) != B_OK) return;
	AmTrackEventUndo*	undo = new AmTrackEventUndo(track, false, topPhrase->Phrase(), event);
	if (undo) mUndoList.push_back(undo);
	/* Remove the container if it's now empty.
	 */
	if (topPhrase->IsEmpty() ) {
		track->RemoveEvent(NULL, topPhrase, NULL, NULL);
		AmTrackEventUndo*	undo = new AmTrackEventUndo(track, false, NULL, topPhrase);
		if (undo) mUndoList.push_back(undo);
	}
}

void _AmTrackRefilterEntry::Print(uint32 tabs) const
{
	for (uint32 t = 0; t < tabs; t++) printf("\t");
	printf("_AmTrackRefilterEntry track %p\n", mTrackId);
	for (uint32 t = 0; t < tabs + 1; t++) printf("\t");
	printf("%ld TrackEventUndo entries:\n", mUndoList.size() );
	for (uint32 k = 0; k < mUndoList.size(); k++) {
		for (uint32 t = 0; t < tabs + 2; t++) printf("\t");
		mUndoList[k]->Print();
	}
}

// #pragma mark -

/*************************************************************************
 * AM-REFILTER-TOOL-SEED
 *************************************************************************/
AmRefilterToolSeed::AmRefilterToolSeed()
		: mGridCache(PPQN)
{
	mFlags = RESTORE_FLAG;
}

AmRefilterToolSeed::AmRefilterToolSeed(const AmRefilterToolSeed& o)
		: mGridCache(PPQN)
{
	mFlags = o.mFlags;
}

AmRefilterToolSeed::~AmRefilterToolSeed()
{
}

AmSelectionsI* AmRefilterToolSeed::MouseDown(	AmSongRef songRef,
												AmToolTarget* target,
												BPoint where)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song) selections = MouseDown(song, target, where);
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	target->TrackWinProperties().SetSelections(selections);
	return NULL;
}

AmSelectionsI* AmRefilterToolSeed::MouseMoved(	AmSongRef songRef,
												AmToolTarget* target,
												BPoint where,
												uint32 code)
{
	ArpASSERT(target);
	AmSelectionsI*	selections = NULL;
	if (IsReady() ) {
		// WRITE TRACK BLOCK
		AmSong*		song = songRef.WriteLock();
		if (song) selections = Move(song, target);
		songRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
	}
	return selections;	
}

void AmRefilterToolSeed::PostMouseMoved(	AmSongRef songRef,
											AmToolTarget* target,
											AmSelectionsI* selections,
											BPoint where)
{
	if (selections && target) {
		// READ TRACK BLOCK
		const AmSong*	song = songRef.ReadLock();
		if (song) target->Perform(song, selections);
		songRef.ReadUnlock(song);
		// END READ TRACK BLOCK
	}
}

AmSelectionsI* AmRefilterToolSeed::MouseUp(	AmSongRef songRef,
											AmToolTarget* target,
											BPoint where)
{
	ArpASSERT(target);
	// WRITE SONG BLOCK
	AmSong*		song = songRef.WriteLock();
	Finished(song);
	songRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
	return NULL;
}

AmToolSeedI* AmRefilterToolSeed::Copy() const
{
	return new AmRefilterToolSeed(*this);
}

void AmRefilterToolSeed::Prepare(	AmSong* song,
									AmToolTarget* target,
									AmSelectionsI* selections,
									BPoint origin,
									AmTime gridTime)
{
	ArpASSERT(target && selections);
	mGridCache = gridTime;
	mEntries.resize(0);

	uint32			trackCount = selections->CountTracks();
	for (uint32 ti = 0; ti < trackCount; ti++) {
		mEntries.push_back(_AmTrackRefilterEntry(song, target, selections, ti));
	}
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].IsEmpty() ) {
			mEntries.erase(mEntries.begin() + k);
			k = 0;
		}
	}
}

bool AmRefilterToolSeed::IsReady()
{
	return mEntries.size() > 0;
}

AmSelectionsI* AmRefilterToolSeed::Move(AmSong* song,
										AmToolTarget* target)
{
	ArpASSERT(IsReady() && song && target);

	if (mFlags&RESTORE_FLAG) {
		for (uint32 k = 0; k < mEntries.size(); k++)
			mEntries[k].UndoEverything(song);
	}

	AmSelectionsI*	selections = PostMove(song, target->TrackWinProperties().Selections() );
	return selections;
}

void AmRefilterToolSeed::Finished(AmSong* song)
{
	if (song && song->UndoContext() ) {
		for (uint32 k = 0; k < mEntries.size(); k++) mEntries[k].Finished(song);
	}
	mEntries.resize(0);
}

void AmRefilterToolSeed::AddPhrase(AmTrack* track, AmPhraseEvent* pe, uint32 flags)
{
	ArpASSERT(track && pe);
	ValidateTrackEntry(track);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (track->Id() == mEntries[k].mTrackId) {
			mEntries[k].AddPhrase(track, pe);
			return;
		}
	}
}

void AmRefilterToolSeed::AddEvent(	AmTrack* track, AmPhraseEvent* topPhrase,
									AmEvent* event, uint32 flags)
{
	ArpASSERT(track && topPhrase && event);
	ValidateTrackEntry(track);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (track->Id() == mEntries[k].mTrackId) {
			mEntries[k].AddEvent(track, topPhrase, event);
			return;
		}
	}
}

void AmRefilterToolSeed::DeleteEvent(	AmTrack* track, AmPhraseEvent* topPhrase,
										AmEvent* event, uint32 flags)
{
	ArpASSERT(track && topPhrase && event);
	ValidateTrackEntry(track);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (track->Id() == mEntries[k].mTrackId) {
			mEntries[k].DeleteEvent(track, topPhrase, event);
			return;
		}
	}
}

AmSelectionsI* AmRefilterToolSeed::PostMove(AmSong* song, AmSelectionsI* oldSelections)
{
	ArpASSERT(song);
	AmSelectionsI*		newSelections = AmSelectionsI::NewSelections();
	if (!newSelections) return NULL;
	
	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].PostMove(song, newSelections);

	return newSelections;
}

AmSelectionsI* AmRefilterToolSeed::MouseDown(	AmSong* song,
												AmToolTarget* target,
												BPoint where)
{
	ArpASSERT(song && target);
	mGridCache = target->TrackWinProperties().GridTime();
	/* If the mouse was clicked on an existing event, then prepare it to be
	 * refiltered and do nothing more.  If I can't find an event exactly where the
	 * user checked, look to see if there's one at my quantized position.
	 */
	track_id		tid;
	AmSelectionsI*	selections = target->TrackWinProperties().Selections();
	for (uint32 ti = 0; (tid = target->TrackWinProperties().OrderedTrackAt(ti).TrackId()) != 0; ti++) {
		AmTrack*	track = song->Track(tid);
		if (track) {
			AmPhraseEvent*	topPhrase;
			int32			extraData;
			AmEvent*		event = target->EventAt(track, where, &topPhrase, &extraData);
			if (!event) event = target->EventAt(track, new_event_time(where.x, target, mGridCache), where.y, &topPhrase, &extraData);
			if (event) {
				/* If there are previous selections and the user is holding down the
				 * modifier key, add the new event to the previous selections.
				 */
				if (selections && (modifiers() & B_SHIFT_KEY) ) {
					selections->AddEvent(tid, topPhrase, event, extraData);
				/* If there are no current selections, or this item is not
				 * already selected, select this event.
				 */
				} else if (!selections || !(selections->IncludesEvent(tid, topPhrase, event)) ) {
					if (!(selections = AmSelectionsI::NewSelections()) ) return NULL;
					selections->AddEvent(tid, topPhrase, event, extraData);
				/* Finally, we have a selections and this event is included.  Make sure
				 * the entry contains the extraData I just found up above.  This allows users
				 * to click on different parts of the event to change different properties, for
				 * targets that support that behaviour.
				 */
				} else {
					if (selections) selections->SetExtraData(tid, topPhrase, event, extraData);
				}
			}
		}
	}
	/* Now that I've set up the selections, I want to prepare them for refiltering.
	 */
	if (selections) Prepare(song, target, selections, where, mGridCache);
	return selections;
}

void AmRefilterToolSeed::ValidateTrackEntry(AmTrack* track)
{
	ArpASSERT(track);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (track->Id() == mEntries[k].mTrackId) return;
	}
	mEntries.push_back(_AmTrackRefilterEntry(track->Song(), track->Id() ) );
}