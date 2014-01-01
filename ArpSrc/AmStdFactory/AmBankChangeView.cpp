/* AmProgramChangeView.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ArpKernel/ArpDebug.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewFactory.h"

#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

#include "AmStdFactory/AmBankChangeView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

/* The default height of the program change view.
 */
static const float		I_ARP_PREFERREDHEIGHT		= 25;

static const char*		BANKCHANGEINFO_STR			= "BankChangeInfo";
static const char*		BANKCHANGEDATA_STR			= "BankChangeData";

static const int32		BANK_XDATA					= 0x00000001;
static const int32		PROGRAM_XDATA				= 0x00000002;

/*************************************************************************
 * _AM-BANK-TARGET
 * The tool API implementation for the program change view.
 *************************************************************************/
class _AmBankTarget : public AmToolTarget
{
public:
	_AmBankTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);
	~_AmBankTarget();
	
	virtual bool		IsInteresting(const AmEvent* event) const;
	virtual BRect		RectFor(const AmEvent* event, AmRange eventRange) const;
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

private:
	AmPerformer					mPerformer;
	/* This is a fake program change that can be populated and sent to the
	 * EventWidth() method.  How this is done will probably change once I'm
	 * displaying banks, as well.
	 */
	mutable AmProgramChange*	mFakePc;
	/* This a temporary pointer to my current device.  The device is grabbed
	 * from the track, and NULL'd out once the function grabbing it has finished.
	 * If any function wants EventWidth() to find the width of the pc based
	 * on the actual name of the program, they need to make sure to populate
	 * this var before calling that method.
	 */
	mutable ArpCRef<AmDeviceI>	mDevice;

	/* Answer the width of the supplied event, based on the current
	 * font in my view.
	 */
	float EventWidth(const AmBankChange* bc) const;
	BRect BankRectFor(const AmBankChange* event, AmRange eventRange) const;
	void SetProgramTransform(	const AmTrack& track, AmPhraseEvent& topPhrase,
								AmBankChange* event, const am_trans_params& params);
	/* A convenience for getting the program change event in a bank event.
	 */
	AmProgramChange* ProgramChange(const AmBankChange* bc) const;
};

/*************************************************************************
 * AM-BANK-CHANGE-INFO-VIEW
 *************************************************************************/
AmBankChangeInfoView::AmBankChangeInfoView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI* property,
											TrackViewType viewType)
		: AmTrackInfoView(frame, BANKCHANGEINFO_STR, songRef, trackWinProps, viewType)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmBankChangeInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmBankChangeInfoView::DrawOn(BRect clip, BView* view)
{
	AmTrackInfoView::DrawOn(clip, view);

	float	bottom = Bounds().bottom;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString("Bank", BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

// #pragma mark -

/*************************************************************************
 * AM-BANK-CHANGE-DATA-VIEW
 *************************************************************************/
AmBankChangeDataView::AmBankChangeDataView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI& viewProp,
											TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, BRect(0,0,0,0), BANKCHANGEDATA_STR,
				B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
		  mMeasureBg(NULL), mDevice(NULL)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmBankTarget(trackWinProps, this);
	
	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
}

AmBankChangeDataView::~AmBankChangeDataView()
{
}

void AmBankChangeDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmBankChangeDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll(this);
	mSongRef.RemoveObserverAll(this);
}

void AmBankChangeDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	AddAsObserver();
}

void AmBankChangeDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmBankChangeDataView::MessageReceived(BMessage* msg)
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
														AmNotifier::OTHER_EVENT_OBS,
														range);
				Invalidate();
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmBankChangeDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

static ArpCRef<AmDeviceI> get_device(const AmTrack* track)
{
	if (!track) return NULL;
	AmFilterHolderI*		holder = track->Filter(DESTINATION_PIPELINE);
	if ( !holder || !holder->Filter() ) return NULL;
	return holder->Filter()->Device();
}

static ArpCRef<AmBankI> get_bank(ArpCRef<AmDeviceI> device, const AmBankChange* be)
{
	if (!device || !be) return NULL;
	return device->Bank(be->BankNumber() );
}

void AmBankChangeDataView::PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	mDevice = get_device(track);
}

void AmBankChangeDataView::PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	mDevice = NULL;
}

static void get_bank_label(ArpCRef<AmBankI> bank, const AmBankChange* bc, BString& out)
{
	if (bank) out = bank->Name().String();
	if (out.Length() < 1) out << "Bank " << bc->BankNumber();
}

static void get_program_label(ArpCRef<AmBankI> bank, const AmProgramChange* pc, BString& out)
{
	out << " / " << pc->ProgramNumber() + ( (bank) ? bank->FirstPatchNumber() : 0);
	BString		patchName;
	if (bank) patchName = bank->PatchName(pc->ProgramNumber() );
	if (patchName.Length() > 0) out << " - " << patchName.String();
}

void AmBankChangeDataView::DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
										const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->PHRASE_TYPE || event->Subtype() != event->BANK_SUBTYPE)
		return;
	const AmBankChange*	bc = dynamic_cast<const AmBankChange*>(event);
	if (!bc) return;

	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor() );
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else
		view->SetHighColor( AmPrefs().SelectedColor() );
	
	BPoint				pt( mTarget->TimeConverter().TickToPixel(eventRange.start),
							Bounds().bottom - 2 );
	/* If I can get a string for the patch, draw that.
	 */
	BString				str;
	ArpCRef<AmBankI>	bank = get_bank(mDevice, bc);
	get_bank_label(bank, bc, str);
	AmNode*				node = ( bc->Phrase() ) ? bc->Phrase()->HeadNode() : NULL;
	while (node) {
		if (node->Event()->Type() == node->Event()->PROGRAMCHANGE_TYPE) {
			AmProgramChange*	pc = dynamic_cast<AmProgramChange*>( node->Event() );
			if (pc) {
				get_program_label(bank, pc, str);
				break;
			}
		}				
		node = node->next;
	}
	view->DrawString(str.String(), pt);
}

void AmBankChangeDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range( mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::OTHER_EVENT_OBS,
											range);
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

// #pragma mark -

/*************************************************************************
 * _AM-PROGRAM-TARGET
 *************************************************************************/
_AmBankTarget::_AmBankTarget(	AmTrackWinPropertiesI& trackWinProps,
								BView* view)
		: AmToolTarget(trackWinProps, view, 200, 200),
		  mFakePc(new AmProgramChange), mDevice(NULL)
{
}

_AmBankTarget::~_AmBankTarget()
{
	if (mFakePc) mFakePc->Delete();
}

bool _AmBankTarget::IsInteresting(const AmEvent* event) const
{
	ArpASSERT(event);
	return (event->Type() == event->PHRASE_TYPE && event->Subtype() == event->BANK_SUBTYPE);
}

BRect _AmBankTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + EventWidth( dynamic_cast<const AmBankChange*>(event) );
	r.bottom = r.bottom -1;
	return r;
}

AmEvent* _AmBankTarget::InterestingEventAt(	const AmTrack* track,
											const AmPhraseEvent& topPhrase,
											const AmPhrase& phrase,
											AmTime time,
											float y,
											int32* extraData) const
{
	/* Because I grab my bank for this method, MAKE SURE to NULL it out
	 * before returning.
	 */
	mDevice = get_device(track);
	/* The program change view seeks from the back forwards.  This is
	 * because of the nature of the visual display of program changes --
	 * which a program change is a single point in time, it's name in
	 * the window might span several beats.  If the user clicks on
	 * multiple overlapping program changes, we want the 'top most one,'
	 * which is the one latest in time, to be what's activated.
	 */ 
	AmNode*		node = phrase.TailNode();
	BPoint		pt(mMtc.TickToPixel(time), y);
	while (node) {
		ArpASSERT(node->Event() != NULL);
		AmRange		eventRange = topPhrase.EventRange( node->Event() );
		if ( ( IsInteresting( node->Event() ) )
				&& (time >= eventRange.start) ) {
			BRect	wholeR = RectFor(node->Event(), eventRange);
			if (wholeR.Contains(pt)) {
				BRect	bankR = BankRectFor(dynamic_cast<AmBankChange*>(node->Event()), eventRange);
				if (bankR.Contains(pt)) *extraData = BANK_XDATA;
				else *extraData = PROGRAM_XDATA;
				mDevice = NULL;
				return node->Event();
			}
		}
		node = node->prev;
	}
	mDevice = NULL;
	return NULL;
}

AmEvent* _AmBankTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
 	ArpCRef<AmDeviceI>	device = track.Device();
	if (!device) return NULL;
	ArpCRef<AmBankI>	bank = device->Bank(uint32(0));
	if (!bank) return NULL;
	AmEvent*			event = bank->NewBankSelection();
	if (!event) return NULL;
	event->SetStartTime(time);
	return event;
}

int32 _AmBankTarget::MoveYValueFromPixel(float y) const
{
	return 0;
}

void _AmBankTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
									const AmEvent* event,
									AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	*y = 0;
}

void _AmBankTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	*yDelta = 0;
}

void _AmBankTarget::SetMove(	AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	if (!event) return;

	AmTime			newStart = originalX + deltaX;
	if (newStart < 0) newStart = 0;
	if (newStart != topPhrase.EventRange(event).start)
		topPhrase.SetEventStartTime(event, newStart);
}

void _AmBankTarget::GetOriginalTransform(	AmEvent* event,
											am_trans_params& params) const
{
	params.original_x = 0;
	params.original_y = 0;
	AmBankChange*	bc = dynamic_cast<AmBankChange*>(event);
	if (!bc) return;

	if (params.extra_data == PROGRAM_XDATA) {
		AmProgramChange*	pc = ProgramChange(bc);
		if (pc) {
			params.original_x = pc->ProgramNumber();
			params.original_y = pc->ProgramNumber();
		}
	} else {
		params.original_x = bc->BankNumber();
		params.original_y = bc->BankNumber();
	}
}

void _AmBankTarget::GetDeltaTransform(	BPoint origin, BPoint where,
										am_trans_params& params) const
{
	/* The number of pixels to traverse before I will change values.
	 */
	float	pixels = 3;
	params.delta_x = (int32) ((where.x - origin.x) / pixels);
	params.delta_y = (int32) ((where.y - origin.y) / pixels);
}

uint32 _AmBankTarget::SetTransform(	const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									const am_trans_params& params)
{
	if (params.extra_data == PROGRAM_XDATA) {
		SetProgramTransform(track, topPhrase, dynamic_cast<AmBankChange*>(event), params);
		return 0;
	}
	AmBankChange*		bc = dynamic_cast<AmBankChange*>(event);
	if (!bc) return 0;
	ArpCRef<AmDeviceI>	device = track.Device();
	if (!device) return 0;
	int32				newBank = params.original_x + params.delta_x - params.delta_y;
	uint32				bankCount = device->CountBanks();
	if (bankCount < 1) return 0;
	if (newBank >= (int32)bankCount) newBank = (int32)bankCount - 1;
	if (newBank < 0) newBank = 0;
	if (bc->BankNumber() == (uint32)newBank) return 0;
	ArpCRef<AmBankI>	bank = device->Bank((uint32)newBank);
	if (!bank) return 0;

	bc->SetTo(bank);
	return 0;
}

uint32 _AmBankTarget::SetTransform(	const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									BPoint where,
									const am_trans_params& params)
{
	return 0;
}

void _AmBankTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	const AmTrack*		track = song->Track(TrackWinProperties().OrderedTrackAt(0).TrackId());
	if (!track) return;
	AmFilterHolderI*	output = track->Filter(OUTPUT_PIPELINE);
	if (!output) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	if (!event) return;

	AmTime		lastTime = 0;
	AmEvent*	e = event;
	while (e) {
		if (e->StartTime() > lastTime) lastTime = e->StartTime();
		e = e->NextEvent();
	}

	AmNoteOn*	on = new AmNoteOn(64, 127, lastTime + 1);
	if (on) {
		on->SetDuration(PPQN);
		on->SetNextFilter(output);
		event = event->MergeEvent(on);
	}

	mPerformer.SetBPM(song->BPM() );
	mPerformer.Play(event);
}

// Convenience function to find the width of the data we are working with.
float _AmBankTarget::EventWidth(const AmBankChange* bc) const
{
	if (!mView) return 0;
	if (!bc) return 0;
	/* If I can get a string for the patch, draw that.
	 */
	BString				str;
	ArpCRef<AmBankI>	bank = get_bank(mDevice, bc);
	get_bank_label(bank, bc, str);
	AmProgramChange*	pc = ProgramChange(bc);
	if (pc) get_program_label(bank, pc, str);
	return mView->StringWidth( str.String() );
}

BRect _AmBankTarget::BankRectFor(const AmBankChange* event, AmRange eventRange) const
{
	ArpASSERT(mView && event);
	BRect				r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	
	BString				str;
	ArpCRef<AmBankI>	bank = get_bank(mDevice, event);
	get_bank_label(bank, event, str);
	float				w = mView->StringWidth( str.String() );
	
	r.right = r.left + w;
	r.bottom = r.bottom -1;
	return r;
}

void _AmBankTarget::SetProgramTransform(const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmBankChange* event,
										const am_trans_params& params)
{
	if (!event || !event->Phrase()) return;
	int32				maxProgram = 127;
	/* Get the max program based on the current bank.
	 * ACTUALLY, DEVICES CAN'T SET THE NUMBER OF PATCHES, SO THIS
	 * IS NOT VALID YET.
	 */
#if 0
	if (event) {
		ArpCRef<AmDeviceI>	device = track.Device();
		if (device) {
			ArpCRef<AmBankI>	bank = device->Bank(event);
			if (bank) maxProgram = bank->CountPatches();
		}
	}
#endif
	int32				newProgram = params.original_x + params.delta_x - params.delta_y;
	if (newProgram > maxProgram) newProgram = maxProgram;
	if (newProgram < 0  ) newProgram = 0;
	AmProgramChange*	pc = ProgramChange(event);
	if (pc) pc->SetProgramNumber(newProgram);
}

AmProgramChange* _AmBankTarget::ProgramChange(const AmBankChange* bc) const
{
	ArpASSERT(bc);
	if (!bc) return NULL;
	AmNode*				node = ( bc->Phrase() ) ? bc->Phrase()->HeadNode() : NULL;
	while (node) {
		if (node->Event()->Type() == node->Event()->PROGRAMCHANGE_TYPE) {
			AmProgramChange*	pc = dynamic_cast<AmProgramChange*>( node->Event() );
			if (pc) return pc;
		}				
		node = node->next;
	}
	return NULL;
}
