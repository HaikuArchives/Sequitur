/* SeqSongTitleMatrixView.cpp
 */
#include <stdio.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpInlineTextView.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "Sequitur/SeqSongTitleMatrixView.h"
#include "Sequitur/SequiturDefs.h"

static const uint32		EDIT_START_MSG	= 'strt';
static const uint32		SHOW_POPUP_MSG	= 'ppup';
static const uint32		OPEN_MSG		= 'Iopn';
static const uint32		REMOVE_MSG		= 'Irmv';
static const char*		STICKY_STR		= "sticky";

/*************************************************************************
 * SEQ-SONG-TITLE-MATRIX-VIEW
 *************************************************************************/
SeqSongTitleMatrixView::SeqSongTitleMatrixView(	BRect frame,
												AmSongRef songRef)
		: inherited(frame,
					"title_matrix",
					B_FOLLOW_ALL,
					B_WILL_DRAW),
		  mSongRef(songRef), mHsb(NULL),
		  mDownTime(-1), mDownTrackId(0), mEditRunner(NULL),
		  mTextCtrl(NULL), mEditTrackId(0), mSticky(false)

{
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongIndexMatrixView::SeqSongIndexMatrixView() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) FillMetrics(song);
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
}

SeqSongTitleMatrixView::~SeqSongTitleMatrixView()
{
}

void SeqSongTitleMatrixView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	mSongRef.AddObserver(this, AmSong::TRACK_CHANGE_OBS);
	mSongRef.AddObserver(this, AmTrack::TITLE_CHANGE_OBS);
}

void SeqSongTitleMatrixView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mSongRef.RemoveObserverAll(this);
}

void SeqSongTitleMatrixView::Draw(BRect clip)
{
	BView* into = this;
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>(Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqSongTitleMatrixView::GetPreferredSize(float* width, float* height)
{
	*width = 0;
	*height = 0;
	for (uint32 k = 0; k < mMetrics.size(); k++) {
		float	mw = 2 + StringWidth(mMetrics[k].mName.String() ) + 2;
		if (mw > *width) *width = mw;
	}
}

void SeqSongTitleMatrixView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case AmTrack::TITLE_CHANGE_OBS: {
			BRect			invalid = arp_invalid_rect();
			track_id		tid;
			BRect			b(Bounds() );
			for (uint32 k = 0; msg->FindPointer(SZ_TRACK_ID, k, & tid) == B_OK; k++) {
				const char*	title;
				if (msg->FindString(SZ_TRACK_TITLE, k, &title) == B_OK) {
					_SeqTitleMetric*	metric = MetricFor(tid);
					if (metric) {
						metric->mName = title;
						invalid = arp_merge_rects(invalid, BRect(b.left, metric->mTop, b.right, metric->mBottom) );
					}
				}
			}
			if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
		} break;
		case AmSong::TRACK_CHANGE_OBS:
			TrackChangeReceived(msg);
			break;
		case EDIT_START_MSG: {
			track_id		tid;
			if (msg->FindPointer(SZ_TRACK_ID, &tid) == B_OK) {
				bool	sticky = true;
				if (msg->FindBool(STICKY_STR, &sticky) != B_OK) sticky = true;
				StartEdit(tid, sticky);
			}
		} break;
		case INLINE_FINALUPDATE_MSG:
			StopEdit();
			break;
		case SHOW_POPUP_MSG:
			if (mDownTime >= 0) {
				StopTimer();
				track_id	tid;
				if (msg->FindPointer(SZ_TRACK_ID, &tid) == B_OK)
					ShowMenu(tid);
				mDownTime = -1;
			}
			break;
		case OPEN_MSG: {
			track_id		tid;
			if (msg->FindPointer(SZ_TRACK_ID, &tid) == B_OK) {
				BMessage		openMsg(SHOWTRACKWIN_MSG);
				openMsg.AddPointer(SZ_TRACK_ID, tid);
				Window()->PostMessage(&openMsg);
			}
		} break;
		case REMOVE_MSG: {
			track_id		tid;
			if (msg->FindPointer(SZ_TRACK_ID, &tid) == B_OK)
				RemoveTrack(tid);
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void SeqSongTitleMatrixView::MouseDown(BPoint where)
{
	mDownTrackId = 0;
	/* We watch all mouse events while the text editor is shown
	 * (to remove it when the mouse moves out of this view), so
	 * ignore any button presses during that time.
	 */
	BRect				b(Bounds() );
	_SeqTitleMetric*	metric = MetricAt(where);
	if (mTextCtrl) {
		printf("Mouse down: "); where.PrintToStream();
		printf("Bounds: "); b.PrintToStream();
		if (metric) {
			BRect	r(b.left, metric->mTop, b.right, metric->mBottom);
			if (!(r.Contains(where))) {
				SetEventMask(0, 0);
				StopEdit();
				StopTimer();
				mDownTime = -1;
			}
		}
		return;
	}
	
	printf("Mouse down: "); where.PrintToStream();
	
	StopTimer();
	if (!metric) return;
	mDownTrackId = metric->mTrackId;
	
	int32	buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	if (buttons&B_SECONDARY_MOUSE_BUTTON) {
		ShowMenu(metric->mTrackId);
		mDownTime = -1;
	} else {
		bigtime_t	doubleClickTime;
		get_click_speed(&doubleClickTime);
		doubleClickTime *= 2;
		BMessage	menuMsg(SHOW_POPUP_MSG);
		menuMsg.AddPointer(SZ_TRACK_ID, metric->mTrackId);
		StartTimer(menuMsg, doubleClickTime);
		mDownTime = system_time();
		
		SetMouseEventMask(B_POINTER_EVENTS,
						  B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY);
	}
}

void SeqSongTitleMatrixView::MouseMoved(BPoint where,
										uint32 code,
										const BMessage* message)
{
	if (!mSticky && !(Bounds().Contains(where))) {
		SetEventMask(0, 0);
		StopEdit();
		StopTimer();
		mDownTime = -1;
	}
}

void SeqSongTitleMatrixView::MouseUp(BPoint where)
{
	if (mDownTime >= 0 && Bounds().Contains(where) ) {
		bigtime_t	doubleClickTime;
		get_click_speed( &doubleClickTime );
		doubleClickTime *= 2;
		bigtime_t	remainingTime = doubleClickTime - (system_time() - mDownTime);
		
		mDownTime = -1;
		
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		
		if (remainingTime <= 0) {
			StopTimer();
			StartEdit(mDownTrackId, false);
			
		} else {
			BMessage	msg(EDIT_START_MSG);
			msg.AddBool(STICKY_STR, true);
			msg.AddPointer(SZ_TRACK_ID, mDownTrackId);
			StartTimer(msg, remainingTime);
		}
	}
}

void SeqSongTitleMatrixView::WindowActivated(bool state)
{
	inherited::WindowActivated(state);
	if (!state) {
		SetEventMask(0, 0);
		StopEdit();
		StopTimer();
		mDownTime = -1;
	}
}

void SeqSongTitleMatrixView::SetHorizontalScrollBar(BScrollBar* sb)
{
	mHsb = sb;
}

void SeqSongTitleMatrixView::SetupScrollBars(bool horizontal, bool vertical)
{
	if (horizontal && mHsb) {
		arp_setup_horizontal_scroll_bar(mHsb, this);
	}
}

void SeqSongTitleMatrixView::StartEdit(track_id tid, bool sticky)
{
	// If I've lost the active state while waiting, don't start editing.
	if (!Window() || !Window()->IsActive() ) return;
	_SeqTitleMetric*	metric = MetricFor(tid);
	if (!metric) return;
	
	StopEdit(false);
	StopTimer();
	mSticky = sticky;
	BRect		b = Bounds();
	b.top = metric->mTop;
	b.bottom = metric->mBottom;
	float		borderL = 0;
	float		fh = arp_get_font_height(this);
	BPoint		origin(borderL + 2, metric->Line1(fh) );
	
	if (!mTextCtrl) {
		BFont font;
		GetFont(&font);
		mTextCtrl = new ArpInlineTextView(BMessenger(this),
										  "text:edit", &font,
										  origin.x, b.right-2, origin.y);
	}
	if (!mTextCtrl) return;
	
	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	
	mTextCtrl->MoveOver(origin.x, b.right-2, origin.y);
	mTextCtrl->SetViewColor( Prefs().Color( AM_DATA_BG_C ) );
	mTextCtrl->SetHighColor( Prefs().Color( AM_DATA_FG_C ) );
	mTextCtrl->SetText(metric->mName.String());
	AddChild(mTextCtrl);
	mEditTrackId = tid;
}

void SeqSongTitleMatrixView::StopEdit(bool keepChanges)
{
	if (!mTextCtrl) return;

	if (keepChanges && mTextCtrl->HasChanged() ) {
		// WRITE TRACK BLOCK
		AmSong*		song = mSongRef.WriteLock();
		AmTrack*	track = song ? song->Track(mEditTrackId) : NULL;
		if (track) track->SetTitle( mTextCtrl->Text() );
		mSongRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
	}
	
	SetEventMask(0, 0);
		
	mTextCtrl->RemoveSelf();
	delete mTextCtrl;
	mTextCtrl = NULL;
	mEditTrackId = 0;
}

void SeqSongTitleMatrixView::FillMetrics(const AmSong* song)
{
	ArpASSERT(song);
	mMetrics.resize(0);
	const AmTrack*	track;
	float			top = 0;
	for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
		float		bottom  = top + track->PhraseHeight();
		int32		index = track->SongIndex();
		if (index >= 0) {
			mMetrics.push_back( _SeqTitleMetric(top, bottom, track->Id(), track->Name() ) );
			top = bottom + 1;
		}
	}
}

void SeqSongTitleMatrixView::RemoveTrack(track_id tid)
{
	ArpASSERT(tid);
	// WRITE SONG BLOCK
	AmSong*		song = mSongRef.WriteLock();
	if (song) song->RemoveTrack(tid);
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void SeqSongTitleMatrixView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(Prefs().Color(AM_DATA_BACKDROP_C) );
	view->FillRect(clip);
	float		fh = arp_get_font_height(view);
	
	for (uint32 k = 0; k < mMetrics.size(); k++) {
		if (mMetrics[k].mTop <= clip.bottom && mMetrics[k].mBottom >= clip.top)
			mMetrics[k].DrawOn(view, clip, fh);
	}
}

static status_t lowest_position(const BMessage& msg, int32* lowest)
{
	status_t	err;
	int32		position;
	if ( (err = msg.FindInt32("position", 0, lowest)) != B_OK) return err;
	for (int32 k = 1; msg.FindInt32("position", k, &position) == B_OK; k++) {
		if (position < *lowest) *lowest = position;
	}
	return B_OK;
}

void SeqSongTitleMatrixView::TrackChangeReceived(BMessage* msg)
{
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongIndexMatrixView::TrackChangeReceived() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) FillMetrics(song);
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK

	int32		position;
	if (lowest_position(*msg, &position) != B_OK) return;

	float		top = 0;
	if (uint32(position) < mMetrics.size() ) top = mMetrics[position].mTop;
	BRect		b = Bounds();
	Invalidate( BRect(b.left, top, b.right, b.bottom) );
}

_SeqTitleMetric* SeqSongTitleMatrixView::MetricAt(BPoint pt, int32* index)
{
	if (index) *index = -1;
	for (uint32 k = 0; k < mMetrics.size(); k++) {
		if (pt.y >= mMetrics[k].mTop && pt.y <= mMetrics[k].mBottom) {
			if (index) *index = int32(k);
			return &(mMetrics[k]);
		}
	}
	return NULL;
}

_SeqTitleMetric* SeqSongTitleMatrixView::MetricFor(track_id tid, int32* index)
{
	if (index) *index = -1;
	for (uint32 k = 0; k < mMetrics.size(); k++) {
		if (mMetrics[k].mTrackId == tid) {
			if (index) *index = int32(k);
			return &(mMetrics[k]);
		}
	}
	return NULL;
}

void SeqSongTitleMatrixView::StartTimer(const BMessage& msg, bigtime_t delay)
{
	StopTimer();
	mEditRunner = new BMessageRunner(BMessenger(this), &msg, delay, 1);
}

void SeqSongTitleMatrixView::StopTimer()
{
	delete mEditRunner;
	mEditRunner = 0;
}

void SeqSongTitleMatrixView::ShowMenu(track_id tid)
{
	ArpASSERT(tid);
	StopEdit();
	SetMouseEventMask(0, 0);
	BPoint	point;
	uint32	buttons;
	GetMouse(&point, &buttons, false);
	BPopUpMenu*		menu = new  BPopUpMenu( "", TRUE, TRUE, B_ITEMS_IN_COLUMN );
	BMenuItem*		item1 = new BMenuItem("Open", new BMessage(OPEN_MSG) );
	BMenuItem*		item2 = new BMenuItem("Edit name", new BMessage(EDIT_START_MSG) );
	BMenuItem*		item3 = new BMenuItem("Remove", new BMessage(REMOVE_MSG) );
	if (menu && item1 && item2 && item3) {
		if (item1->Message() ) item1->Message()->AddPointer(SZ_TRACK_ID, tid);
		if (item2->Message() ) item2->Message()->AddPointer(SZ_TRACK_ID, tid);
		if (item3->Message() ) item3->Message()->AddPointer(SZ_TRACK_ID, tid);
		menu->SetFontSize(10);
		menu->AddItem(item1);
		menu->AddItem(item2);
		menu->AddItem(item3);
		menu->SetTargetForItems(this);
		BRect	r(point, point);
		menu->Go(ConvertToScreen(point), true, false, ConvertToScreen(r), true);
	}
}

/*************************************************************************
 * _SEQ-TITLE-METRIC
 *************************************************************************/
_SeqTitleMetric::_SeqTitleMetric()
		: mTop(0), mBottom(0), mTrackId(0)
{
}

_SeqTitleMetric::_SeqTitleMetric(const _SeqTitleMetric& o)
		: mTop(o.mTop), mBottom(o.mBottom), mTrackId(o.mTrackId),
		  mName(o.mName)
{
}

_SeqTitleMetric::_SeqTitleMetric(	float top, float bottom, track_id trackId,
									const char* name)
		: mTop(top), mBottom(bottom), mTrackId(trackId), mName(name)
{
}

_SeqTitleMetric& _SeqTitleMetric::operator=(const _SeqTitleMetric& o)
{
	mTop = o.mTop;
	mBottom = o.mBottom;
	mTrackId = o.mTrackId;
	mName = o.mName;
	return *this;
}

static rgb_color background_color()
{
	return Prefs().Color(AM_ARRANGE_BG_C);
}

static void draw_background(BView* view, BRect b, rgb_color bgc,
							float borderL, float borderR, float bottomIndent)
{
	view->SetHighColor( bgc );
	view->FillRect( BRect( b.left + borderL - 1, b.top + 10, b.right - borderR, b.bottom - bottomIndent - 3) );
	view->SetLowColor( bgc );
	/* Shade the edges - black outline
	 */
	view->SetHighColor(0, 0, 0);
	view->StrokeLine( BPoint(b.left + borderL - 1, b.top + 2), BPoint(b.right - borderR, b.top + 2) );
	view->FillRect( BRect( b.left + borderL - 1, b.bottom - bottomIndent + 1, b.right - borderR, b.bottom - bottomIndent + 3) );
	/* Shade the edges - lighten the top
	 */
	view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_2_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.top + 3), BPoint(b.right - borderR, b.top + 3) );
	view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_1_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.top + 4), BPoint(b.right - borderR, b.top + 4) );
	view->SetHighColor( view->LowColor() );
	view->StrokeLine( BPoint(b.left + borderL, b.top + 5), BPoint(b.right - borderR, b.top + 5) );
	view->SetHighColor( tint_color( view->LowColor(), B_LIGHTEN_1_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.top + 6), BPoint(b.right - borderR, b.top + 6) );
	view->SetHighColor( tint_color( view->LowColor(), B_LIGHTEN_2_TINT ) );
	view->FillRect( BRect( b.left + borderL, b.top + 7, b.right - borderR, b.top + 8) );
	view->SetHighColor( tint_color( view->LowColor(), B_LIGHTEN_1_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.top + 9), BPoint(b.right - borderR, b.top + 9) );
	/* Shade the edges - darken the bottom
	 */
	view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_1_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.bottom - bottomIndent - 2), BPoint(b.right - borderR, b.bottom - bottomIndent - 2) );
	view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_2_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.bottom - bottomIndent - 1), BPoint(b.right - borderR, b.bottom - bottomIndent - 1) );
	view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_3_TINT ) );
	view->StrokeLine( BPoint(b.left + borderL, b.bottom - bottomIndent + 0), BPoint(b.right - borderR, b.bottom - bottomIndent + 0) );
}

void _SeqTitleMetric::DrawOn(BView* view, BRect clip, float fontHeight)
{
	BRect		b(clip.left, mTop, clip.right, mBottom);
	rgb_color	bgc = background_color();
	float		borderL = 0, borderR = 0;	
	float		bottomIndent = 5;
	float		stringIndent = 4;
	draw_background(view, b, bgc, borderL, borderR, bottomIndent);
	/* Draw the label
	 */
	float		line1 = Line1(fontHeight);
	float		bottom;
	if (line1 < mBottom) bottom = line1;
	else bottom = b.bottom - bottomIndent - stringIndent;
	
	view->SetLowColor(bgc);
	view->SetHighColor( Prefs().Color(AM_ARRANGE_FG_C) );
//	view->DrawString( mName.String(), BPoint(borderL + 2, b.bottom - bottomIndent - stringIndent) );
	view->DrawString( mName.String(), BPoint(borderL + 2, bottom) );
}

float _SeqTitleMetric::Line1(float fontHeight) const
{
	float		topIndent = 2;
	float		line1 = mTop + fontHeight + topIndent;
	return line1;
}
