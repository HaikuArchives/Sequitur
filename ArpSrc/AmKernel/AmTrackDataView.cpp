/* AmTrackDataView.cpp
 */
#include <stdio.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmGraphicEffect.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmDeleteService.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolControls.h"
#include "AmKernel/AmToolKeyHandler.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmToolSeeds.h"

/********************************************************
 * AM-TRACK-DATA-VIEW
 ********************************************************/
AmTrackDataView::AmTrackDataView(	AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI& viewProp,
									TrackViewType viewType,
									BRect frame,
									const char *name,
									uint32 resizeMask,
									uint32 flags)
		: inherited(frame, name, resizeMask, flags),
		  mSongRef(songRef), mTrackWinProps(trackWinProps), mMtc(trackWinProps.TimeConverter()),
		  mTarget(0), mActiveTool(0),
		  mFactorySignature(viewProp.Signature() ), mViewName(viewProp.Name() ), mViewType(viewType),
		  mHeadBackground(0), mScrollRunner(0), mFlags(0), mSongPosition(-1),
		  mToolControls(0), mToolGraphic(0), mToolKeys(0), mTransStep(1), mTransCount(0)
{
	mSecondaryBase = Prefs().Color(AM_DATA_BG_C);
}

AmTrackDataView::~AmTrackDataView()
{
	delete mScrollRunner;
	delete mToolControls;
	delete mToolGraphic;
	delete mToolKeys;
	delete mHeadBackground;
	
	for (uint32 k = 0; k < mPrevToolGraphics.size(); k++)
		delete mPrevToolGraphics[k];
	mPrevToolGraphics.resize(0);
}

void AmTrackDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void AmTrackDataView::Draw(BRect clip)
{
	BRect		realClip(clip);
	float		leftFudge = 0;
	float		rightFudge = 0;
	if (mTarget) mTarget->GetFudgeFactor(&leftFudge, &rightFudge);
	BRect		b = Bounds();
	if (leftFudge == AM_FUDGE_TO_EDGE) realClip.left = b.left;
	else realClip.left -= leftFudge;
	if (rightFudge == AM_FUDGE_TO_EDGE) realClip.right = b.right;
	else realClip.right += rightFudge;
	/* If the left bounds are below zero, that can cause a problem
	 * for some methods in AmPhrase, so I avoid that.
	 */
	if (realClip.left < 0) realClip.left = 0;
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing( this, realClip );
	DrawOn(realClip, into);
	if (cache) cache->FinishDrawing(into);
}

/* Default to 0, 0.  Not sure why, that's what the old ArpView
 * class was doing so I'm maintaining it.
 */
void AmTrackDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = 0;
}

void AmTrackDataView::KeyDown(const char *bytes, int32 numBytes)
{
	if (!mTarget || numBytes != 1) {
		inherited::KeyDown(bytes, numBytes);
		return;
	}

	if (mToolKeys && mToolKeys->CanHandle(bytes[0])) {
		// WRITE TOOL BLOCK
		AmTool*		tool = mActiveTool.WriteLock();
		if (tool) tool->KeyDown(mSongRef, mTarget, bytes[0]);
		mActiveTool.WriteUnlock(tool);
		// END WRITE TOOL BLOCK
		return;
	}
	
	switch ( bytes[0] ) {
		case B_DELETE:
			DeleteSelectedEvents();
			break;
		case B_LEFT_ARROW:
			if (modifiers()&B_CONTROL_KEY) MoveEventsBy(-QuantizeTime(), 0);
			else if (modifiers()&B_OPTION_KEY) TransformEventsBy(-mTransStep, 0);
			break;
		case B_RIGHT_ARROW:
			if (modifiers()&B_CONTROL_KEY) MoveEventsBy(QuantizeTime(), 0);
			else if (modifiers()&B_OPTION_KEY) TransformEventsBy(mTransStep, 0);
			else SelectRightEvent();
			break;
		case B_UP_ARROW:
			if (modifiers()&B_CONTROL_KEY) MoveEventsBy(0, 1);
			else if (modifiers()&B_OPTION_KEY) TransformEventsBy(0, -mTransStep);
			break;
		case B_DOWN_ARROW:
			if (modifiers()&B_CONTROL_KEY) MoveEventsBy(0, -1);
			else if (modifiers()&B_OPTION_KEY) TransformEventsBy(0, mTransStep);
			break;
		inherited::KeyDown(bytes, numBytes);
			break;
	}

	/* The mTransStep is the number of steps to take in the transform function.
	 * This is a simple little tweaked algorithm designed to work well both for
	 * small values and large.
	 */
	if (mTransCount == 15) mTransStep += 1;
	else if (mTransCount == 25) mTransStep += 5;
	else if (mTransCount >= 35) mTransStep += 5;
	if (mTransStep > 1000) mTransStep = 1000;
	mTransCount++;
}

void AmTrackDataView::KeyUp(const char *bytes, int32 numBytes)
{
	inherited::KeyUp(bytes, numBytes);
	mTransStep = 1;
	mTransCount = 0;
}

void AmTrackDataView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case AM_ORDERED_TRACK_MSG:
			break;
		case AM_SATURATION_MSG:
			break;
		case AM_SCROLL_MSG:
			HandleScrollMsg(msg);
			break;
		case AM_STOP_MSG:
			if (mTarget) mTarget->Stop();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmTrackDataView::MouseDown(BPoint where)
{
	mLastPt.Set(-1, -1);
	mActiveTool.SetTo(NULL);
	delete mToolControls;
	mToolControls = 0;
	delete mToolKeys;
	mToolKeys = 0;
	if (!mTarget) return;
	uint32			button;
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	GetMouse(&where, &button, false);
	if (!Bounds().Contains(where) ) return;
	MakeFocus(true);

	/* Any current tool graphic automatically gets pushed off the
	 * stack on the mouse down.
	 */
	if (mToolGraphic) {
		if (mToolGraphic->IsFinished() ) delete mToolGraphic;
		else mPrevToolGraphics.push_back(mToolGraphic);
		mToolGraphic = NULL;
	}
	delete mScrollRunner;
	mScrollRunner = NULL;
	
	mActiveTool = AmGlobals().Tool(button, modifiers());
	// WRITE TOOL BLOCK
	AmTool*		tool = mActiveTool.WriteLock();
	if (tool) {
		tool->Prepare(mFactorySignature, mViewName);
		tool->MouseDown(mSongRef, mTarget, where);
		/* Start receiving a message that tells me to scroll myself
		 * if the mouse is out of bounds.  This is done AFTER the
		 * tool is given its mouse down message on purpose:  It gives
		 * the tool the opportunity to set its flags before any are
		 * used.  This is helpful if the tool relies on other tools,
		 * but doesn't know which one it's using until the user presses
		 * down.
		 */
		const AmToolSeedI*	seed = tool->CurrentSeed();
		if (seed && seed->Flags()&seed->AM_SCROLL_FLAG) {
			BMessage		msg(AM_SCROLL_MSG);
			msg.AddBool("x", true);
			if (mViewType == PRI_VIEW) msg.AddBool("y", true);
			mScrollRunner = new BMessageRunner( BMessenger(this), &msg, 100000, -1);
		}
		/* Let the tool construct any tool controls it has.
		 */
		mToolControls = tool->NewControlList();
		if (mToolControls) mToolControls->Prepare(this, Bounds(), where);
		/* If the tool has a graphic that corresponds with my
		 * signature, go ahead and instatiate it.
		 */
		mToolGraphic = tool->NewGraphic(mFactorySignature, mViewName);
		if (mToolGraphic) mToolGraphic->Begin(this, where);
		/* An optional handler for keypresses.
		 */
		mToolKeys = tool->NewToolKeyHandler();
	}
	mActiveTool.WriteUnlock(tool);
	// END WRITE TOOL BLOCK
}

void AmTrackDataView::MouseUp(BPoint where)
{
	MouseCleanup(where);
	
}

void AmTrackDataView::MouseMoved(	BPoint where,
									uint32 code,
									const BMessage *a_message)
{
	/* Sometimes, this view might not receive a mouse up for
	 * whatever reason, which can cause the scroll runner to
	 * lock into permanent autoscroll.  This simply makes sure
	 * the scroll runner has been deleted if it's not valid.
	 */
	int32		button;
	if (Window()->CurrentMessage()->FindInt32("buttons", &button) != B_OK) button = 0;
	if (button == 0) MouseCleanup(where);
	if (!mTarget) return;
	/* There is a bug that seems to be caused by the ArpBitmapCache
	 * stuff, which causes the view to continually receive MouseMoved()
	 * events, even when the mouse is not moving (note that this does not
	 * always occur, it seems more frequent on some systems then others).
	 * This doesn't seem to be a problem in R6, although I'm not entirely
	 * certain about that.
	 */
	if (mLastPt == where) return;
	mLastPt = where;

	if (button != 0) {
		// WRITE TOOL BLOCK
		AmTool*		tool = mActiveTool.WriteLock();
		if (tool) {
			tool->MouseMoved(mSongRef, mTarget, where, code);
			if (mToolGraphic && tool->Id() == mToolGraphic->OwnerId() )
				mToolGraphic->MouseMoved(where, code);
		}
		mActiveTool.WriteUnlock(tool);
		// END WRITE TOOL BLOCK
	}
	if (mToolControls) mToolControls->MouseMoved(this, where);
}

void AmTrackDataView::DrawSongPosition(AmTime time, bool redraw)
{
	BRect		b(Bounds() );
	BRect		invalid(-1, b.top, -1, b.bottom);

	if (mSongPosition >= 0) invalid.left = invalid.right = mSongPosition;
	mSongPosition = mMtc.TickToPixel(time);
	if (mSongPosition >= 0) {
		if (invalid.left == -1 || mSongPosition < invalid.left)
			invalid.left = mSongPosition;
		if (invalid.right == -1 || mSongPosition > invalid.right)
			invalid.right = mSongPosition;
	}
	if (redraw && invalid.left >= 0 && invalid.right >= 0)
		Invalidate(invalid);
}

BMessage* AmTrackDataView::ConfigurationData()
{
	return NULL;
}

BString AmTrackDataView::FactorySignature() const
{
	return mFactorySignature;
}

BString AmTrackDataView::ViewName() const
{
	return mViewName;
}

static void mix_in(const rgb_color& c1, const rgb_color c2, float mix, rgb_color& mixed)
{
	if (mix <= 0) mixed = c1;
	else if (mix >= 1) mixed = c2;
	else {
		mixed.red = uint8(c1.red + ((c2.red - c1.red) * mix));
		mixed.green = uint8(c1.green + ((c2.green - c1.green) * mix));
		mixed.blue = uint8(c1.blue + ((c2.blue - c1.blue) * mix));
	}
}

rgb_color AmTrackDataView::EventColor(uint8 velocity)
{
	if (velocity >= 127) return mEventColor;
	if (velocity <= 0) return mLowEventColor;

	rgb_color		c;
	mix_in(mLowEventColor, mEventColor, (float)velocity / 127, c);
	return c;
}

void AmTrackDataView::AddBackground(ArpBackground* background)
{
	if (mHeadBackground) mHeadBackground->AddTail(background);
	else mHeadBackground = background;
}

static bool has_finished(vector<AmGraphicEffect*>& graphics)
{
	for (uint32 k = 0; k < graphics.size(); k++) {
		if (graphics[k]->IsFinished() ) {
			vector<AmGraphicEffect*>::iterator		i = graphics.begin() + k;
			delete (*i);
			graphics.erase(i);
			return true;
		}
	}
	return false;
}

void AmTrackDataView::DrawOn(BRect clip, BView* view)
{
	view->SetHighColor( Prefs().Color(AM_DATA_BG_C) );
	view->FillRect(clip);
	if (mHeadBackground) mHeadBackground->DrawAllOn(view, clip);

	if (!mTarget) return;

	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmTrackDataView::DrawOn() read lock\n");
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackWinProps.OrderedTrackAt(0)) : 0;
	if (track) {
		mOrderedSaturation = mTrackWinProps.OrderedSaturation();
		mShadowSaturation = mTrackWinProps.ShadowSaturation();
		PreDrawEventsOn(clip, view, track);
		if (mShadowSaturation > 0) DrawShadowEvents(clip, view, song);
		if (mOrderedSaturation > 0) DrawOrderedEvents(clip, view, song);
		DrawPrimaryEvents(clip, view, track);
		PostDrawEventsOn(clip, view, track);
	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK

	// WRITE TOOL BLOCK
	AmTool*		tool = mActiveTool.WriteLock();
	if (tool) tool->DrawOn(view, clip);
	mActiveTool.WriteUnlock(tool);
	// END WRITE TOOL BLOCK
	
	/* Tool grapahics -- clean up any previous graphics that are finished,
	 * and let everyone draw.
	 */
	while (has_finished(mPrevToolGraphics)) ;
	for (uint32 k = 0; k < mPrevToolGraphics.size(); k++)
		mPrevToolGraphics[k]->DrawOn(view, clip);
	if (mToolGraphic) mToolGraphic->DrawOn(view, clip);

	/* Draw the song position.
	 */
	if (mSongPosition >= 0) {
		view->SetHighColor(0, 0, 0);
		view->StrokeLine( BPoint(mSongPosition, clip.top), BPoint(mSongPosition, clip.bottom) );
	}

	/* And finally, the transparent tool controls draw over everyone else.
	 */
	if (mToolControls) mToolControls->DrawOn(view, clip);
}

void AmTrackDataView::PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
}

void AmTrackDataView::PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
}

void AmTrackDataView::DrawShadowEvents(BRect clip, BView* view, const AmSong* song)
{
	/* Shadow events can draw themselves as translucent if they
	 * wish.  So set that up.
	 */
	drawing_mode	mode = view->DrawingMode();
	if (mShadowSaturation > 0 && mShadowSaturation < 1 ) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->SetLowColor(mSecondaryBase);
	}
	
	const AmTrack*	track;
	for (uint32 k=0; (track = song->Track(k)) != 0; k++) {
		if (!IsOrdered(track->Id() )) DrawTrack(track, clip, view, ARPEVENT_SHADOW);
	}

	view->SetDrawingMode(mode);
}

//#include "AmKernel/AmSelections.h"
void AmTrackDataView::DrawOrderedEvents(BRect clip, BView* view, const AmSong* song)
{
//printf("%s DrawOrderedEvents, selections:\n", Name() );
//AmSelections*	sel = (AmSelections*)mTrackWinProps.Selections();
//if (sel) sel->Print();
	/* Ordered events can draw themselves as translucent if they
	 * wish.  So set that up.
	 */
	drawing_mode	mode = view->DrawingMode();
	if (mOrderedSaturation > 0 && mOrderedSaturation < 1 ) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->SetLowColor(mSecondaryBase);
	}
	
	uint32			count = mTrackWinProps.CountOrderedTracks();
	for (uint32 k = 1; k < count; k++) {
		const AmTrack*	track = song->Track( mTrackWinProps.OrderedTrackAt(k).TrackId() );
		if (track) {
			DrawTrack(track, clip, view, ARPEVENT_ORDERED, mTrackWinProps.Selections() );
		}
	}

	view->SetDrawingMode(mode);
}

void AmTrackDataView::DrawPrimaryEvents(BRect clip, BView* view, const AmTrack* track)
{
	DrawTrack(track, clip, view, ARPEVENT_PRIMARY, mTrackWinProps.Selections() );
}

void AmTrackDataView::DrawTrack(const AmTrack* track,
								BRect clip,
								BView* view,
								int32 properties,
								AmSelectionsI* selections)
{
	AmTime		start = mMtc.PixelToTick(clip.left - 2);
	AmTime		end = mMtc.PixelToTick(clip.right + 2);
	AmNode*		n = track->Phrases().HeadNode();
	while (n && n->Event() && n->StartTime() <= end) {
		if (n->EndTime() >= start) {
			if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
				if (pe && pe->Phrase() ) {
					mEventColor = pe->Phrase()->Color(AmPhrase::FOREGROUND_C);
					mix_in(mEventColor, AmPrefs().Color(AM_DATA_BG_C), 0.75, mLowEventColor);
					DrawPhrase(clip, view, track->Id(), *pe, pe, start, end, properties, selections);
				}
			}
		}
		n = n->next;
	}
}

void AmTrackDataView::DrawPhrase(	BRect clip,
									BView* view,
									track_id trackId,
									const AmPhraseEvent& topPhrase,
									AmPhraseEvent* pe,
									AmTime start,
									AmTime end,
									int32 properties,
									AmSelectionsI* selections)
{
	ArpASSERT(pe && pe->Phrase());
	AmNode*		n = pe->Phrase()->HeadNode();
	if (!n) return;
	AmRange		eventRange = topPhrase.EventRange( n->Event() );
	while (n && eventRange.start <= end) {
		if (eventRange.end >= start) {
			if (mTarget->IsInteresting( n->Event() )) {
				if (selections && selections->IncludesEvent(trackId, &topPhrase, n->Event() ) )
					DrawEvent(view, topPhrase, n->Event(), eventRange, ARPEVENT_SELECTED);
				else
					DrawEvent(view, topPhrase, n->Event(), eventRange, properties);
			} else if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
				AmPhraseEvent*	pe2 = dynamic_cast<AmPhraseEvent*>( n->Event() );
				if (pe2) DrawPhrase(clip, view, trackId, topPhrase, pe2, start, end, properties, selections);
			}
		}
		n = n->next;
		if (n) eventRange = topPhrase.EventRange( n->Event() );
	}
}

void AmTrackDataView::MouseCleanup(BPoint where)
{
	delete mScrollRunner;
	mScrollRunner = 0;
	if (mToolControls) {
		mToolControls->MouseCleanup(this);
		delete mToolControls;
		mToolControls = 0;
	}
	delete mToolKeys;
	mToolKeys = 0;
	
	if (mTarget) {
		// WRITE TOOL BLOCK
		AmTool*		tool = mActiveTool.WriteLock();
		if (tool) tool->MouseUp(mSongRef, mTarget, where);
		mActiveTool.WriteUnlock(tool);
		// END WRITE TOOL BLOCK
		mActiveTool.SetTo(NULL);
	}	
}

void AmTrackDataView::HandleScrollMsg(const BMessage* msg)
{
	bool		scrollToX, scrollToY;
	if (msg->FindBool("x", &scrollToX) != B_OK) scrollToX = false;
	if (msg->FindBool("y", &scrollToY) != B_OK) scrollToY = false;
	if (!scrollToX && !scrollToY) return;
	float		x_delta = 0, y_delta = 0;
	BRect		f = Frame();
	BPoint		where;
	uint32		button;
	GetMouse(&where, &button, false);
	ConvertToParent(&where);

	if( where.x < f.left ) x_delta = where.x - f.left;
	else if( where.x > f.right ) x_delta = where.x - f.right;

	if( where.y < f.top ) y_delta = where.y - f.top;
	else if( where.y > f.bottom ) y_delta = where.y - f.bottom;

	if( x_delta != 0 || y_delta != 0 ) {
		BMessage	msg(AM_SCROLL_MSG);
		if (scrollToX && x_delta != 0) msg.AddFloat( "x", x_delta * 0.5 );
		if (scrollToY && y_delta != 0) msg.AddFloat( "pri_y", y_delta * 0.5 );
		if ( Window() ) Window()->PostMessage( &msg );
	}
}

bool AmTrackDataView::IsOrdered(track_id tid) const
{
	uint32		count = mTrackWinProps.CountOrderedTracks();
	for (uint32 k = 0; k < count; k++) {
		if (mTrackWinProps.OrderedTrackAt(k).TrackId() == tid) return true;
	}
	return false;
}

void AmTrackDataView::DeleteSelectedEvents()
{
	AmSelectionsI*		selections = mTrackWinProps.Selections();
	if (!selections) return;
	// WRITE SONG BLOCK
	AmSong*				song = mSongRef.WriteLock();
	if (song) {
		AmDeleteService	service;
		service.Prepare(song);
		uint32			trackCount = selections->CountTracks();
		for (uint32 ti = 0; ti < trackCount; ti++) {
			AmTrack*		track = NULL;
			track_id		tid;
			AmPhraseEvent*	topPhrase;
			AmEvent*		event;
			for (uint32 ei = 0; selections->EventAt(ti, ei, &tid, &topPhrase, &event) == B_OK; ei++) {
				if (!track) track = song->Track(tid);
				if (!track) break;
				service.Delete(track, event, topPhrase);
			}
		}
		service.Finished(song);
	}
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void AmTrackDataView::MoveEventsBy(AmTime timeDelta, int32 yDelta)
{
	AmSelectionsI*		selections = mTrackWinProps.Selections();
	if (!selections) return;
	// WRITE SONG BLOCK
	AmSong*				song = mSongRef.WriteLock();
	if (song) {
		AmMoveToolSeed	seed;
		seed.Prepare(song, mTarget, selections, BPoint(0, 0), PPQN);
		if (seed.IsReady() ) {
			seed.Move(song, mTarget, timeDelta, yDelta);
			seed.Finished(song);
		}
	}
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void AmTrackDataView::TransformEventsBy(int32 xDelta, int32 yDelta)
{
	AmSelectionsI*		selections = mTrackWinProps.Selections();
	if (!selections) return;
	// WRITE SONG BLOCK
	AmSong*				song = mSongRef.WriteLock();
	if (song) {
		AmTransformToolSeed	seed;
		seed.Prepare(song, mTarget, selections, BPoint(0, 0), PPQN);
		if (seed.IsReady() ) {
			seed.Transform(song, mTarget, xDelta, yDelta);
			seed.Finished(song);
		}
	}
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

AmTime AmTrackDataView::QuantizeTime() const
{
	return mTrackWinProps.GridTime();
}

/**********************************************************************
 * This is all the code for handling navigating events via the keys.
 * It's really tweaky right now.
 **********************************************************************/

void AmTrackDataView::SelectRightEvent()
{
#if 0
	ArpASSERT(mTarget);
	AmSelectionsI*		newSelections = NULL;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmTrackDataView::SelectRightEvent() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackWinProps.OrderedTrackAt(0)) : NULL;
	if (track) {
		AmSelectionsI*	selections = mTrackWinProps.Selections();
		/* If there are no selections, select the first event
		 * starting on or after the left edge of the window.
		 */
		if (!selections || selections->CountEvents() < 1) {
			newSelections = SelectFirstEvent(track);
		/* If there are already events, select the next event
		 * over from the end of the last selected event.  And if
		 * that event isn't visible, select the first visible event.
		 */
		} else {
			/* Find the event to move right from.
			 */
			AmEvent*		event = 0;
			AmPhraseEvent*	container = 0;
			AmEvent*		iteratingEvent;
			AmPhraseEvent*	iteratingContainer;
			for (uint32 k = 0; selections->EventAt(k, &iteratingContainer, &iteratingEvent) == B_OK; k++) {
				if (!event || (iteratingContainer->EventRange(iteratingEvent).start > container->EventRange(event).start) ) {
					event = iteratingEvent;
					container = iteratingContainer;
				}
			}
			if (!event) newSelections = SelectFirstEvent(track);
			else {
				/* Select the next event over.  This has a lot of problems,
				 * like not dealing with operlapping phrases.
				 */
				if (NextRightEvent(track, event, container, &iteratingEvent, &iteratingContainer) == B_OK)
					newSelections = SelectEvent(iteratingEvent, iteratingContainer);
				else newSelections = SelectFirstEvent(track);
			}
		}

	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
	if (newSelections) mTrackWinProps.SetSelections(newSelections);
#endif
}

status_t AmTrackDataView::NextRightEvent(	const AmTrack* track,
											AmEvent* event, AmPhraseEvent* container,
											AmEvent** eventAnswer, AmPhraseEvent** containerAnswer) const
{
#if 0
	ArpASSERT( mTarget );
	ArpASSERT( event && container && container->Phrase() );
	AmNode*		n = container->Phrase()->FindNode(event);
//	AmEvent*	nextEvent = 0;
	/* Simple case, there's an event after the current one.
	 */
	if( n ) n = n->next;
	while( n ) {
		if( mTarget->IsInteresting( n->Event() ) ) {
			*eventAnswer = n->Event();
			*containerAnswer = container;
			return B_OK;
		}
		n = n->next;
	}
#endif
	return B_ERROR;
}

AmSelectionsI* AmTrackDataView::SelectFirstEvent(const AmTrack* track)
{
	ArpASSERT(track);
	AmPhraseEvent*	container = NULL;
	AmEvent*		event = FirstEvent(track, &container);
	if (event) return SelectEvent(event, container);
	else return NULL;
}

AmSelectionsI* AmTrackDataView::SelectEvent(AmEvent* event, AmPhraseEvent* container)
{
return NULL;
#if 0
	ArpASSERT(event && container);
	AmSelectionsI*	selections = AmSelectionsI::NewSelections();
	if (selections) selections->AddEvent(container, event);
	return selections;
#endif
}

/* This method finds the first event that STARTS after my left edge.
 * events that are overhanging into my left border are ignored.  This
 * is an arbitrary decision, but basically it makes sense to me to do
 * it this way because this method is used to select the first visible
 * event, if there isn't already one.  And if I did it the other way,
 * then that overhanging note might actually be measures away, so as users
 * clicked the right arrow the actual events being selected would be far
 * away.
 */
AmEvent* AmTrackDataView::FirstEvent(const AmTrack* track, AmPhraseEvent** answer) const
{
	ArpASSERT(mTarget);
	const AmPhrase&		phrase = track->Phrases();
	AmTime				time = mTarget->TimeConverter().PixelToTick( Bounds().left );
	AmNode*				phraseNode = phrase.FindNode( time, BACKWARDS_SEARCH );
	if( !phraseNode ) return 0;
	AmPhraseEvent*		pe;
	while (phraseNode != 0) {
		if( (phraseNode->Event()->Type() == phraseNode->Event()->PHRASE_TYPE)
				&& (pe = dynamic_cast<AmPhraseEvent*>( phraseNode->Event() ))
				&& pe->Phrase() ) {
			AmNode*		eventNode = pe->Phrase()->FindNode( time, FORWARDS_SEARCH );
			while( eventNode ) {
				if( mTarget->IsInteresting( eventNode->Event() ) ) {
					*answer = pe;
					return eventNode->Event();
				}
				eventNode = eventNode->next;
			}
		}
		phraseNode = phraseNode->next;
	}
	return 0;
}
