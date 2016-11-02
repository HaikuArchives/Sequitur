#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewPropertyI.h"
#include "AmStdFactory/AmControlChangeView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const float	PREFERRED_HEIGHT			= 50;
static const uint8	INITIAL_CC					= 10;
static const uint32	CHANGE_FAC_VIEW_REPORT_MSG	= 'iCVR';
static const char*	RUN_REPORT_PREF				= "ccrep";

#define I_ARP_CC_CHANGE_MSG		'aCcm'
#define STR_CONTROL_CHANGE		"ControlChange"

// The configuration message I generate and that gets passed back to me.
#define ARPMSG_CONFIGDATA		'pCnf'


class _AmActiveControlItem : public BMenuItem
{
public:
	const BFont*		boldFont;

	_AmActiveControlItem(	const char *label, BMessage *message,
							char shortcut = 0, uint32 modifiers = 0)
			: inherited(label, message, shortcut, modifiers),
			  boldFont(0)			{ }

protected:
	virtual	void		GetContentSize(float *width, float *height)
	{
		const char*		l = Label();
		if (!boldFont || !l) {
			inherited::GetContentSize(width, height);
			return;
		}
		font_height		fh;
		boldFont->GetHeight(&fh);
		float			w = boldFont->StringWidth(l),
						h = ceil(fh.ascent + fh.descent + fh.leading);
		if (width) *width = w;
		if (height) *height = h;
	}

	virtual	void		DrawContent()
	{
		if (!boldFont) {
			inherited::DrawContent();
			return;
		}
		Menu()->SetFont(boldFont);
		inherited::DrawContent();
		Menu()->SetFont(be_plain_font);
	}

private:
	typedef BMenuItem	inherited;
};

/*************************************************************************
 * _AM-CONTROL-TARGET
 * The tool API implementation for the control change view.
 *************************************************************************/
class _AmControlTarget : public AmToolTarget
{
public:
	_AmControlTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);
	virtual ~_AmControlTarget();
	
	void SetControlNumber( uchar controlNumber );

	virtual uint32		Flags() const;
	virtual bool		IsInteresting(const AmEvent* event) const;
	virtual bool		IsInteresting(const BMessage* flatEvent) const;
	virtual BRect		RectFor(const AmEvent* event, AmRange eventRange) const;
	virtual AmTime		EventAtFudge() const;
	virtual AmEvent*	InterestingEventAt(	const AmTrack* track,
											const AmPhraseEvent& topPhrase,
											const AmPhrase& phrase,
											AmTime time,
											float y,
											int32* extraData) const;

	virtual AmEvent* NewEvent(const AmTrack& track, AmTime time, float y);
	// Moving
	virtual int32 MoveYValueFromPixel(float y) const;
	virtual void GetMoveValues(	const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmTime* x, int32* y) const;
	virtual void GetMoveDelta(	BPoint origin, BPoint where,
								AmTime* xDelta, int32* yDelta) const;
	virtual void SetMove(		AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags);
	// Transforming
	virtual void		GetOriginalTransform(	AmEvent* event,
												am_trans_params& params) const;
	virtual void		GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const;
	virtual uint32		SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params);
	virtual uint32		SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										BPoint where,
										const am_trans_params& params);
	// Performing
	virtual void Perform(const AmSong* song, const AmSelectionsI* selections); 
	// Hacks
	virtual void PrepareForPaste(AmEvent* event);

private:
	AmPerformer		mPerformer;
	/* The current control change number of my view.
	 */
	uchar			mControlNumber;

	/* Given a y position, answer a control value.  This method assumes the mView
	 * is set.
	 */
	uchar ControlValueFromPixel(float y) const;
	int32 BoundlessControlValueFromPixel(float y) const;
};

/*************************************************************************
 * _AM-CONTROL-MENU
 *************************************************************************/
class _AmControlMenu : public BMenu
{
public:
	_AmControlMenu(	const char* title,
					AmSongRef songRef,
					AmTrackRef trackRef,
					BMessenger target,
					uchar initialCc);

	virtual void	AttachedToWindow();

	void			Build();

	void			RunReport(const AmTrack* track);
	
private:
	typedef BMenu	inherited;
	AmSongRef		mSongRef;
	AmTrackRef		mTrackRef;
	BMessenger		mTarget;
	uchar			mInitialCc;
	
	bool			IsBuilt() const;
	void			Build(const AmTrack* track);
	status_t		AddControlItem(uint32 number, const char* label);
};

/*************************************************************************
 * AM-CONTROL-CHANGE-INFO-VIEW
 *************************************************************************/
AmControlChangeInfoView::AmControlChangeInfoView(	BRect frame,
													AmSongRef songRef,
													AmTrackWinPropertiesI& trackWinProps,
													const AmViewPropertyI* property,
													TrackViewType viewType)
		: inherited(frame, STR_CONTROL_CHANGE_INFO, songRef, trackWinProps, viewType),
		  mCc(INITIAL_CC), mCachedControlMenu(0)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
	if( property->Configuration() ) SetConfiguration( property->Configuration() );
}

void AmControlChangeInfoView::SetConfiguration(const BMessage* config)
{
	int32	tmp;
	if (config->FindInt32(STR_CONTROL_CHANGE, &tmp) == B_OK) {
		mCc = (uchar)tmp;
		BMenu*	menu = ControlMenu();
		if (menu) {
			BMenuItem*	item;
			for (int32 index = 0; (item = menu->ItemAt( index )); index++) {
				if (index == mCc) item->SetMarked(false);
				else item->SetMarked(true);
			}
		}
	}
}

void AmControlChangeInfoView::AttachedToWindow()
{
	inherited::AttachedToWindow();
}

void AmControlChangeInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmControlChangeInfoView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case I_ARP_CC_CHANGE_MSG:
			int32	tmp;
			if (msg->FindInt32(STR_CONTROL_CHANGE, &tmp) == B_OK) {
				mCc = (uchar)tmp;
				mTrackWinProps.PostMessageToDataView(*msg, this);
				Invalidate();
			}
			break;
		case ARPMSG_CONFIGDATA:
			SetConfiguration(msg);
			break;
				
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmControlChangeInfoView::ControlActivated(int32 code)
{
	if (!mCachedControlMenu) return;

	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							RUN_REPORT_PREF, &pref) != B_OK)
		pref = 0;
	if (pref == 0) {
		mCachedControlMenu->RunReport(0);
		return;
	}
	
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmControlChangeInfoView::ControlActivated() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackWinProps.OrderedTrackAt(0)) : NULL;
	if (track) mCachedControlMenu->RunReport(track);
	mSongRef.ReadUnlock(song);
	// END READ TRACK BLOCK	
}

void AmControlChangeInfoView::DrawOn(BRect clip, BView* view)
{
	inherited::DrawOn(clip, view);
	BRect		b = Bounds();
	float		bottom = b.bottom;
	BString		cname = MenuStringForControl(mCc);
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString( "Control", BPoint(2, bottom - 12) );
	view->DrawString( cname.String(), BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

BPopUpMenu* AmControlChangeInfoView::NewPropertiesMenu() const
{
	BPopUpMenu*		menu = inherited::NewPropertiesMenu();
	if (!menu) return menu;
	mCachedControlMenu = NewControlMenu();

	if (!mCachedControlMenu) return menu;
	BMenuItem*		item;
	if (mCachedControlMenu->CountItems() < 1 ) {
		item = new _AmActiveControlItem( "<track has no output filter>", new BMessage('null') );
		if (item) {
			item->SetEnabled( false );
			mCachedControlMenu->AddItem(item);
		}
	}
	item = new BMenuItem(mCachedControlMenu);
	if (!item) {
		delete mCachedControlMenu;
		mCachedControlMenu = NULL;
		return menu;
	}
	menu->AddSeparatorItem();
	menu->AddItem(item);
	return menu;
}

_AmControlMenu* AmControlChangeInfoView::NewControlMenu() const
{
	_AmControlMenu*			menu = new _AmControlMenu( "Control change",
														mSongRef,
														mTrackWinProps.OrderedTrackAt(0),
														BMessenger(this),
														mCc );
	if( !menu ) return 0;
	menu->SetRadioMode( true );
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont( be_plain_font );
	menu->Build();
	return menu;
}

BString AmControlChangeInfoView::MenuStringForControl(uint32 control) const
{
	if (control > 127) return BString("?");
	BMenu*		menu = ControlMenu();
	if (!menu) return BString( am_control_name(control) );

	BMenuItem*	item;
	for (int32 index = 0; (item = menu->ItemAt( index )); index++) {
		if (index == (int32)control) return BString( item->Label() );
	}
	return BString( am_control_name(control) );
}

BMenu* AmControlChangeInfoView::ControlMenu() const
{
	return mCachedControlMenu;
}

// #pragma mark -

/*************************************************************************
 * AM-CONTROL-CHANGE-DATA-VIEW
 *************************************************************************/
AmControlChangeDataView::AmControlChangeDataView(	BRect frame,
													AmSongRef songRef,
													AmTrackWinPropertiesI& trackWinProps,
													const AmViewPropertyI& viewProp,
													TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, STR_CONTROL_CHANGE_DATA,
				B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL), mCc(INITIAL_CC)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmControlTarget(trackWinProps, this);
	if (viewProp.Configuration() ) Configure(viewProp.Configuration() );

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
}

void AmControlChangeDataView::SetControlNumber(uchar controlNumber)
{
	mCc = (uchar)controlNumber;
	_AmControlTarget*	target = dynamic_cast<_AmControlTarget*>(mTarget);
	if (target) target->SetControlNumber(controlNumber);
	Invalidate();
}

void AmControlChangeDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmControlChangeDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll( this );
	mSongRef.RemoveObserverAll( this );
}

void AmControlChangeDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	mPt2.y = Bounds().bottom - 1;
	mScale = (mPt2.y -1) / 127;	
	AddAsObserver();
}

void AmControlChangeDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmControlChangeDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

void AmControlChangeDataView::MessageReceived(BMessage *msg)
{
	if (mTarget && mTarget->HandleMessage(msg)) return;

	switch (msg->what) {
		case AM_ORDERED_TRACK_MSG: {
			int32		order;
			if (msg->FindInt32("track_order", &order) == B_OK && order == 0) {
				mCachedPrimaryTrack.RemoveObserverAll(this);
				mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
				if (mMeasureBg) mMeasureBg->SetTrackRef(mCachedPrimaryTrack);
				BRect		b = Bounds();
				AmRange		range( mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
				mCachedPrimaryTrack.AddRangeObserver(	this,
														AmNotifier::CONTROL_CHANGE_OBS,
														range);
				Invalidate();
			}
		} break;
		case I_ARP_CC_CHANGE_MSG:
			int32	tmp;
			if (msg->FindInt32(STR_CONTROL_CHANGE, &tmp) == B_OK) {
				SetControlNumber( (uchar)tmp );
			}
			break;
		case ARPMSG_CONFIGDATA:
			Configure(msg);
			break;
				
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

BMessage* AmControlChangeDataView::ConfigurationData()
{
	BMessage	*msg = new BMessage(ARPMSG_CONFIGDATA);
	if (msg == 0) return 0;
	msg->AddInt32(STR_CONTROL_CHANGE, mCc);
	return msg;
}

void AmControlChangeDataView::PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	inherited::PreDrawEventsOn(clip, view, track);
	/* If pan, do some special drawing.
	 */
	BRect	b = Bounds();
	if (mCc == 10) {
		view->SetHighColor( Prefs().Color(AM_MEASURE_BEAT_C) );
		/* With the current size of the view, the center wasn't quite coming out
		 * to cc value 64, so I'm fudging it by one pixel.
		 */
		float	y = b.top + ((b.bottom - b.top) / 2) - 1;
		view->StrokeLine( BPoint(clip.left, y), BPoint(clip.right, y) );
	}
}

void AmControlChangeDataView::Configure(const BMessage* msg)
{
	int32	tmp;
	if (msg->FindInt32(STR_CONTROL_CHANGE, &tmp) == B_NO_ERROR) {
		SetControlNumber( (uchar)tmp );
	}
}

// Assumes pt1 and pt2 have been set properly.
void AmControlChangeDataView::DrawEvent(BView* view, const AmPhraseEvent& topPhrase,
										const AmEvent* event, AmRange eventRange, int32 properties)
{
	if ( (event->Type() == event->CONTROLCHANGE_TYPE)
			&& (((AmControlChange*)event)->ControlNumber() == mCc) ) {

		// Set the properties for the note
		if (properties&ARPEVENT_PRIMARY)
			view->SetHighColor(EventColor() );
		else if (properties&ARPEVENT_SHADOW)
			view->SetHighColor( AmPrefs().ShadowColor() );
		else
			view->SetHighColor( AmPrefs().SelectedColor() );

		mPt1.x = mPt2.x = mMtc.TickToPixel(eventRange.start);

		mPt1.y = abs((int)(mPt2.y - (mScale * ((AmControlChange*)event)->ControlValue())));
		if (mPt1.y < 0) mPt1.y = 0;

		// FIX: should do a lineArray thing!  Cool!
		view->StrokeLine(mPt1, mPt2);
	}
}

void AmControlChangeDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range( mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::CONTROL_CHANGE_OBS,
											range);
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

// #pragma mark -

/*************************************************************************
 * _AM-CONTROL-TARGET
 *************************************************************************/
_AmControlTarget::_AmControlTarget(	AmTrackWinPropertiesI& trackWinProps,
									BView* view)
		: AmToolTarget(trackWinProps, view),
		  mControlNumber(INITIAL_CC)
{
}

_AmControlTarget::~_AmControlTarget()
{
}

void _AmControlTarget::SetControlNumber( uchar controlNumber )
{
	mControlNumber = controlNumber;
}

uint32 _AmControlTarget::Flags() const
{
	return DRAG_TIME_ONLY;
}

bool _AmControlTarget::IsInteresting(const AmEvent* event) const
{
	assert( event );
	if( event->Type() != event->CONTROLCHANGE_TYPE ) return false; 
	const AmControlChange*	cc = dynamic_cast<const AmControlChange*>( event );
	if( !cc ) return false;
	if( cc->ControlNumber() != mControlNumber ) return false;
	return true;
}

bool _AmControlTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::CONTROLCHANGE_TYPE;
}

BRect _AmControlTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + 1;
	r.bottom = r.bottom -1;
	return r;
}

AmTime _AmControlTarget::EventAtFudge() const
{
	/* My fudge is three pixels.
	 */
	return mMtc.PixelToTick(3);
}

AmEvent* _AmControlTarget::InterestingEventAt(	const AmTrack* track,
												const AmPhraseEvent& topPhrase,
												const AmPhrase& phrase,
												AmTime time,
												float y,
												int32* extraData) const
{
	AmNode*		n = phrase.ChainHeadNode(time);
	if (!n) return NULL;
	/* Since control changes are single pixels, it can be a bit tricky
	 * for users to hit exactly the right pixel.  Compensate for this
	 * by finding the closest control change within a given fudge factor.
	 */
	AmTime		fudge = EventAtFudge();
	AmEvent*	closest = NULL;
	/* As soon as I've hit events that are within range of where I'm
	 * looking, I set this flag.  This lets me know to end the search
	 * as soon as I'm out of range.
	 */
	bool		beenInRange = false;
	while (n) {
		AmRange		eventRange = topPhrase.EventRange( n->Event() );
		if( (eventRange.start >= time - fudge) && (eventRange.start <= time + fudge) ) {
			beenInRange = true;
			if( IsInteresting( n->Event() ) ) {
				if (!closest)
					closest = n->Event();
				else if ( abs(time - eventRange.start) < abs(time - topPhrase.EventRange(closest).start) )
					closest = n->Event();
			}
		} else {
			if (beenInRange) return closest;
		}
		n = n->next;
	}
	return closest;
}

AmEvent* _AmControlTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	assert( mView );
	return new AmControlChange( mControlNumber,
								ControlValueFromPixel( y ),
								time);
}

int32 _AmControlTarget::MoveYValueFromPixel(float y) const
{
	return ControlValueFromPixel(y);
}

void _AmControlTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
										const AmEvent* event,
										AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmControlChange*		ccEvent = dynamic_cast<const AmControlChange*>(event);
	if (ccEvent) *y = ccEvent->ControlValue();
	else *y = 0;
}

void _AmControlTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	/* I don't want the normal bounded transformation so that the move
	 * tool can 'flatten' all the events as you drag to the edge of the view.
	 */
	int32		originVal = BoundlessControlValueFromPixel(origin.y);
	int32		whereVal = BoundlessControlValueFromPixel(where.y);
	*yDelta = (whereVal - originVal);
}

void _AmControlTarget::SetMove(	AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	AmControlChange*		ccEvent = dynamic_cast<AmControlChange*>(event);
	if (!ccEvent) return;
//	if (flags&TRANSFORM_X) {

	AmTime			newStart = originalX + deltaX;
	if (newStart < 0) newStart = 0;
	if (newStart != topPhrase.EventRange(event).start)
		topPhrase.SetEventStartTime(ccEvent, newStart);

//	if (flags&TRANSFORM_Y) {
		int32			newVal = originalY + deltaY;
		if (newVal > 127) newVal = 127;
		else if (newVal < 0) newVal = 0;
		ccEvent->SetControlValue(newVal);
//	}
}

void _AmControlTarget::GetOriginalTransform(AmEvent* event,
											am_trans_params& params) const
{
	AmControlChange*	cc = dynamic_cast<AmControlChange*>(event);
	if (!cc) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = cc->ControlValue();
		params.original_y = cc->ControlValue();
	}
}

void _AmControlTarget::GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const
{
	params.delta_x = 0;
	params.delta_y = (int32) (where.y - origin.y);
}

uint32 _AmControlTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									const am_trans_params& params)
{
	AmControlChange*	cc = dynamic_cast<AmControlChange*>(event);
	if (!cc) return 0;
	int32			newValue = params.original_y - params.delta_y;
	if (newValue > 127) newValue = 127;
	if (newValue < 0  ) newValue = 0;
	cc->SetControlValue(newValue);
	return 0;
}

uint32 _AmControlTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									BPoint where,
									const am_trans_params& params)
{
	AmControlChange*	cc = dynamic_cast<AmControlChange*>(event);
	if (!cc) return 0;
	if (params.flags&TRANSFORM_Y)
		cc->SetControlValue( ControlValueFromPixel(where.y) );
	return 0;
}

void _AmControlTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	const AmTrack*		track = song->Track(TrackWinProperties().OrderedTrackAt(0).TrackId());
	if (!track) return;
	AmFilterHolderI*	output = track->Filter(OUTPUT_PIPELINE);
	if (!output) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	if (!event) return;

	AmNoteOn*	on = new AmNoteOn(64, 100, 0);
	if (!on) {
		event->DeleteChain();
		return;
	}
	on->SetNextFilter(output);
	event = on->MergeEvent(event);
	if (!event) return;
	AmEvent*	e = event;
	while ( e->NextEvent() ) e = e->NextEvent();
	if (e) on->SetEndTime( e->EndTime() );
	if (on->Duration() < PPQN / 8) on->SetEndTime( on->StartTime() + (PPQN / 8) );

	mPerformer.SetBPM(song->BPM() );
	mPerformer.Play(event);
}

void _AmControlTarget::PrepareForPaste(AmEvent* event)
{
	ArpASSERT(event);
	AmControlChange*		ccEvent = dynamic_cast<AmControlChange*>(event);
	if (!ccEvent) return;
	ccEvent->SetControlNumber(mControlNumber);
}

uchar _AmControlTarget::ControlValueFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	if( y < b.top ) return 127;
	else if( y > (b.bottom - 1) ) return 0;

	return (uchar) (127 - ((127 / (b.Height() -1 )) * y));
}

int32 _AmControlTarget::BoundlessControlValueFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	return (int32) (127 - ((127 / (b.Height() -1 )) * y));
}

// #pragma mark -

/*************************************************************************
 * _AM-CONTROL-MENU
 *************************************************************************/
_AmControlMenu::_AmControlMenu(	const char* title,
								AmSongRef songRef,
								AmTrackRef trackRef,
								BMessenger target,
								uchar initialCc)
		: BMenu(title), mSongRef(songRef), mTrackRef(trackRef),
		  mTarget(target), mInitialCc(initialCc)
{
}

void _AmControlMenu::AttachedToWindow()
{
	if (!IsBuilt()) Build();
	inherited::AttachedToWindow();
}

void _AmControlMenu::Build()
{
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("_AmControlMenu::Build() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) Build(track);
	mSongRef.ReadUnlock(song);
	// END READ TRACK BLOCK	
}

static void _control_report(const AmPhrase* phrase, uint8* active)
{
	if (!phrase) return;
	AmNode*		node = phrase->HeadNode();
	while (node) {
		if (node->Event() ) {
			if (node->Event()->Type() == node->Event()->PHRASE_TYPE) {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(node->Event() );
				if (pe) _control_report(pe->Phrase(), active);
			} else if (node->Event()->Type() == node->Event()->CONTROLCHANGE_TYPE) {
				AmControlChange*	cc = dynamic_cast<AmControlChange*>(node->Event() );
				if (cc) active[cc->ControlNumber()] = 1;
			}
		}
		node = node->next;
	}
}

void _AmControlMenu::RunReport(const AmTrack* track)
{
	const BFont*			f = be_bold_font;
	ArpVALIDATE(f, return);
	
	if (CountItems() != AM_CONTROLLER_SIZE) return;

	uint8					active[AM_CONTROLLER_SIZE];
	uint32					k;
	for (k = 0; k < AM_CONTROLLER_SIZE; k++) active[k] = 0;
	if (track) _control_report(&(track->Phrases()), active);

	for (k = 0; k < AM_CONTROLLER_SIZE; k++) {
		_AmActiveControlItem*	item = dynamic_cast<_AmActiveControlItem*>(ItemAt(k));
		if (item) {
			if (active[k] > 0) item->boldFont = f;
			else item->boldFont = 0;
		}
	}
}

bool _AmControlMenu::IsBuilt() const
{
	int32		count = CountItems();
	if (count > 1) return true;
	if (count == 1) {
		BMenuItem*	item = ItemAt(0);
		if (!item) return false;
		if (!item->Message()) return false;
		if (item->Message()->what == 'null') return false;
		return true;
	}
	return false;
}

void _AmControlMenu::Build(const AmTrack* track)
{
	AmFilterHolderI* 		holder = track->Filter(DESTINATION_PIPELINE);
	ArpCRef<AmDeviceI>		device = NULL;
	if (holder && holder->Filter()) device = holder->Filter()->Device();
	
	RemoveItems(0, CountItems(), true);
	for (uint32 k = 0; k < AM_CONTROLLER_SIZE; k++) {
		BString			cn;
		if (device) cn = device->ControlName(k, true);
		if (cn.Length() < 1) cn << k;
		const char*		label = NULL;
		if (cn.Length() > 0) label = cn.String();
		if (!label) label = am_control_name(k);
		if (!label) {
			cn << k;
			label = cn.String();
		}
		AddControlItem(k, label);
	}
}

status_t _AmControlMenu::AddControlItem(uint32 number, const char* label)
{
	BMessage*	msg = new BMessage(I_ARP_CC_CHANGE_MSG);
	if (!msg) return B_NO_MEMORY;
	msg->AddInt32(STR_CONTROL_CHANGE, (int32)number);

	BMenuItem*	item = new _AmActiveControlItem(label, msg);
	if (!item) {
		delete msg;
		return B_NO_MEMORY;
	}
	item->SetTarget(mTarget);
	if (number == mInitialCc) item->SetMarked(true);
	AddItem(item);
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * AM-CONTROL-CHANGE-PREF-VIEW
 ***************************************************************************/
AmControlChangePrefView::AmControlChangePrefView(	BRect f, BMessage* prefs,
													const BString& facSig,
													const BString& facKey)
		: inherited(f, prefs, facSig, facKey), mRunReport(0)
{
	AddViews();
}

void AmControlChangePrefView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mRunReport) mRunReport->SetTarget(this);
}

void AmControlChangePrefView::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case CHANGE_FAC_VIEW_REPORT_MSG:
			if (mRunReport) {
				mPrefs.SetInt32Preference(	mFacSig.String(), mFacKey.String(),
											RUN_REPORT_PREF, mRunReport->Value());
			}
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmControlChangePrefView::AddViews()
{
	inherited::AddViews();
	/* Run Report control
	 */
	float				y = AddY();

	const char*			label = "Run report";
	float				lW = StringWidth(label);
	BRect				f(0, y, lW + Prefs().Size(CHECK_BOX_X) + 10, y + Prefs().Size(CHECK_BOX_Y));
	mRunReport = new BCheckBox(f, "rr", label, new BMessage(CHANGE_FAC_VIEW_REPORT_MSG) );
	if (!mRunReport) return;

	int32				i32;
	if (mPrefs.GetInt32Preference(mFacSig.String(), mFacKey.String(), RUN_REPORT_PREF, &i32) != B_OK)
		i32 = 0;
	mRunReport->SetValue(i32);

	AddChild(mRunReport);

}
