/* SeqSongIndexMatrixView.cpp
 */
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "Sequitur/SeqSongIndexMatrixView.h"
#include "Sequitur/SeqSongSelections.h"
#include "Sequitur/SequiturDefs.h"

static const BBitmap*		gMuteOn		= NULL;
static const BBitmap*		gMuteOff	= NULL;
static const BBitmap*		gSoloOn		= NULL;
static const BBitmap*		gSoloOff	= NULL;

static const char* group_name_from(uint32 groups);

/*************************************************************************
 * SEQ-SONG-INDEX-MATRIX-VIEW
 *************************************************************************/
SeqSongIndexMatrixView::SeqSongIndexMatrixView(	BRect frame,
												AmSongRef songRef,
												float modeWidth)
		: inherited(frame,
					"index_matrix",
					B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
					B_WILL_DRAW),
		  mSongRef(songRef), mModeWidth(modeWidth)
{
	if (!gMuteOn) gMuteOn = ImageManager().FindBitmap(MUTE_ON_IMAGE_STR);
	if (!gMuteOff) gMuteOff = ImageManager().FindBitmap(MUTE_OFF_IMAGE_STR);
	if (!gSoloOn) gSoloOn = ImageManager().FindBitmap(SOLO_ON_IMAGE_STR);
	if (!gSoloOff) gSoloOff = ImageManager().FindBitmap(SOLO_OFF_IMAGE_STR);

	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqSongIndexMatrixView::SeqSongIndexMatrixView() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) FillMetrics(song);
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
}

SeqSongIndexMatrixView::~SeqSongIndexMatrixView()
{
}

void SeqSongIndexMatrixView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	mSongRef.AddObserver(this, AmSong::TRACK_CHANGE_OBS);
	mSongRef.AddObserver(this, AmTrack::MODE_CHANGE_OBS);
	mSongRef.AddObserver(this, AmTrack::GROUP_CHANGE_OBS);
}

void SeqSongIndexMatrixView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mSongRef.RemoveObserverAll(this);
}

void SeqSongIndexMatrixView::Draw(BRect clip)
{
	BView* into = this;
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqSongIndexMatrixView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case AmTrack::MODE_CHANGE_OBS: {
			BRect			invalid = arp_invalid_rect();
			track_id		tid;
			for (uint32 k = 0; msg->FindPointer(SZ_TRACK_ID, k, & tid) == B_OK; k++) {
				int32		flags;
				if (msg->FindInt32("mode flags", &flags) == B_OK) {
					_SeqIndexMetric*	metric = MetricFor(tid);
					if (metric) {
						metric->mModeFlags = flags;
						invalid = arp_merge_rects(invalid, metric->mFrame);
					}
				}
			}
			if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
		} break;
		case AmTrack::GROUP_CHANGE_OBS: {
			BRect			invalid = arp_invalid_rect();
			track_id		tid;
			for (uint32 k = 0; msg->FindPointer(SZ_TRACK_ID, k, & tid) == B_OK; k++) {
				int32		groups;
				if (msg->FindInt32("groups", &groups) == B_OK) {
					_SeqIndexMetric*	metric = MetricFor(tid);
					if (metric) {
						metric->mGroups = groups;
						invalid = arp_merge_rects(invalid, metric->mFrame);
					}
				}
			}
			if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
		} break;
		case AmSong::TRACK_CHANGE_OBS:
			TrackChangeReceived(msg);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void SeqSongIndexMatrixView::MouseDown(BPoint where)
{
	mMouseDown = DOWN_ON_NOTHING;
	mMouseDownIndex = -1;
	mMouseDownOnMode = 0;
	/* There should never NOT be a props -- that's my window
	 * object, so it would be quite odd.
	 */
	SeqSongWinPropertiesI*		props = SongWinProperties();
	if (!props) return;

	_SeqIndexMetric*	metric = MetricAt(where, &mMouseDownIndex);
	if (!metric) {
		props->SetSelections(NULL);
		return;
	}
	mMouseDownOnMode = metric->ModeForPoint(where);
	if (mMouseDownOnMode != 0) {
		mMouseDown = DOWN_ON_MODE;
		return;
	}
	/* If there is a current selections object and it isn't
	 * empty, then I maintain it's time range, regardless of
	 * whether I'm adding to it or replacing it.
	 */
	SeqSongSelections*		selections = props->Selections();
	AmRange					curRange;
	bool					selectionsEmpty = true;
	if (selections && !selections->IsEmpty() ) {
		curRange = selections->TimeRange();
		selectionsEmpty = false;
	}
	
	selections = NULL;
	if (modifiers() & B_SHIFT_KEY) selections = props->Selections();
	if (selections) selections = selections->Copy();
	if (!selections) selections = SeqSongSelections::New();
	if (!selections) return;
	selections->SetTimeRange(curRange);

	if (selections->IncludesTrack(metric->mTrackId) ) {
		mMouseDown = DOWN_ON_SELECTED_TRACK;
		selections->RemoveTrack(metric->mTrackId);
	} else {
		mMouseDown = DOWN_ON_DESELECTED_TRACK;
		selections->AddTrack(metric->mTrackId);
	}
	
	props->SetSelections(selections);
}

void SeqSongIndexMatrixView::MouseMoved(BPoint where,
										uint32 code,
										const BMessage* dragMessage)
{
	int32		buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	if (buttons == 0) {
		if (mMouseDown != DOWN_ON_MODE) mMouseDown = DOWN_ON_NOTHING;
		return;
	}

	if (mMouseDown != DOWN_ON_SELECTED_TRACK && mMouseDown != DOWN_ON_DESELECTED_TRACK)
		return;
	if (mMouseDownIndex < 0) return;
	int32						metricIndex;
	_SeqIndexMetric*			metric = MetricAt(where, &metricIndex);
	if (!metric) return;
	/* There should never NOT be a props -- that's my window
	 * object, so it would be quite odd.
	 */
	SeqSongWinPropertiesI*		props = SongWinProperties();
	if (!props) return;

	/* The mouse down that initiated this drag should have guaranteed
	 * there's a selection object.  If there isn't, there's probably
	 * a memory problem, so I just fail.  I copy so that I'm not modifying
	 * the original selections object, which could cause confusion for
	 * the view updating routines.
	 */
	SeqSongSelections*			selections = props->Selections();
	if (!selections) return;
	selections = selections->Copy();
	if (!selections) return;
	/* If I'm selecting tracks, make sure every track between myself
	 * and the origin is selected.  If I'm deselecting, make sure they're
	 * deselected.
	 */
	bool						changes;
	if (mMouseDown == DOWN_ON_DESELECTED_TRACK)
		changes = SelectBetween(selections, mMouseDownIndex, metricIndex);
	else changes = DeselectBetween(selections, mMouseDownIndex, metricIndex);
	if (changes) props->SetSelections(selections);
	else delete selections;

}

void SeqSongIndexMatrixView::MouseUp(BPoint where)
{
	/* Cache these values so I can immediately wipe clean my
	 * mouse state, just so I don't have to worry about it.
	 */
	uint32	mouseDownOnMode = mMouseDownOnMode;
	int32	cachedIndex = mMouseDownIndex;
	PostMouseUp();
	
	/* There should never NOT be a props -- that's my window
	 * object, so it would be quite odd.
	 */
	SeqSongWinPropertiesI*	props = SongWinProperties();
	if (!props) return;

	int32					metricIndex;
	_SeqIndexMetric*		metric = MetricAt(where, &metricIndex);
	if (!metric) return;
	if (mouseDownOnMode != 0 && cachedIndex == metricIndex
			&& mouseDownOnMode == metric->ModeForPoint(where) ) {
		if (metric->mModeFlags&mouseDownOnMode) metric->mModeFlags &= ~mouseDownOnMode;
		else metric->mModeFlags |= mouseDownOnMode;
		// WRITE TRACK BLOCK
		AmSong*				song = mSongRef.WriteLock();
		if (song) {
			AmTrack*		track = song->Track(metric->mTrackId);
			if (track) {
				if (metric->mModeFlags != track->ModeFlags() )
					track->SetModeFlags(metric->mModeFlags);
			}
		}
		mSongRef.WriteUnlock(song);
		// END WRITE TRACK BLOCK
	}
}

void SeqSongIndexMatrixView::InvalidateTracks(std::vector<track_id>& tracks)
{
	BRect		invalid = arp_invalid_rect();
	for (uint32 k = 0; k < tracks.size(); k++) {
		_SeqIndexMetric*	metric = MetricFor(tracks[k] );
		if (metric) invalid = arp_merge_rects(invalid, metric->mFrame);
	}
	if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
}

void SeqSongIndexMatrixView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(Prefs().Color(AM_DATA_BACKDROP_C) );
	view->FillRect(clip);

	SeqSongWinPropertiesI*		props = SongWinProperties();
	if (!props) return;

	float		fh = arp_get_font_height(view);
	
	for (uint32 k = 0; k < mMetrics.size(); k++) {
		if (mMetrics[k].mFrame.top <= clip.bottom && mMetrics[k].mFrame.bottom >= clip.top)
			mMetrics[k].DrawOn(view, clip, *props, fh);
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

void SeqSongIndexMatrixView::TrackChangeReceived(BMessage* msg)
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
	if (uint32(position) < mMetrics.size() ) top = mMetrics[position].mFrame.top;
	BRect		b = Bounds();
	Invalidate( BRect(b.left, top, b.right, b.bottom) );
}

void SeqSongIndexMatrixView::FillMetrics(const AmSong* song)
{
	ArpASSERT(song);
	mMetrics.resize(0);
	const AmTrack*	track;
	BRect			f(Bounds() );
	f.top = 0;
	for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
		f.bottom = f.top + track->PhraseHeight();
		int32		index = track->SongIndex();
		if (index >= 0) {
			mMetrics.push_back( _SeqIndexMetric(track->Id(), uint32(index),
												track->ModeFlags(), track->Groups(),
												f, mModeWidth) );
			f.top = f.bottom + 1;
		}
	}
}

bool SeqSongIndexMatrixView::SelectBetween(	SeqSongSelections* selections,
											uint32 index1, uint32 index2) const
{
	uint32		low = std::min(index1, index2);
	uint32		high = std::max(index1, index2);
	bool		changes = false;
	for (uint32 k = low; k <= high; k++) {
		if (k >= mMetrics.size() ) return changes;
		if (!(selections->IncludesTrack(mMetrics[k].mTrackId)) ) {
			selections->AddTrack(mMetrics[k].mTrackId);
			changes = true;
		}
	}
	return changes;
}

bool SeqSongIndexMatrixView::DeselectBetween(	SeqSongSelections* selections,
												uint32 index1, uint32 index2) const
{
	uint32		low = std::min(index1, index2);
	uint32		high = std::max(index1, index2);
	bool		changes = false;
	for (uint32 k = low; k <= high; k++) {
		if (k >= mMetrics.size() ) return changes;
		if (selections->IncludesTrack(mMetrics[k].mTrackId) ) {
			selections->RemoveTrack(mMetrics[k].mTrackId);
			changes = true;
		}
	}
	return changes;
}

void SeqSongIndexMatrixView::PostMouseUp()
{
	mMouseDown = DOWN_ON_NOTHING;
	mMouseDownIndex = -1;
	mMouseDownOnMode = 0;
}

_SeqIndexMetric* SeqSongIndexMatrixView::MetricAt(BPoint pt, int32* index)
{
	if (index) *index = -1;
	for (uint32 k = 0; k < mMetrics.size(); k++) {
		if (mMetrics[k].mFrame.Contains(pt) ) {
			if (index) *index = int32(k);
			return &(mMetrics[k]);
		}
	}
	return NULL;
}

_SeqIndexMetric* SeqSongIndexMatrixView::MetricFor(track_id tid, int32* index)
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

SeqSongWinPropertiesI* SeqSongIndexMatrixView::SongWinProperties() const
{
	return dynamic_cast<SeqSongWinPropertiesI*>( Window() );
}

/*************************************************************************
 * _SEQ-INDEX-METRIC
 *************************************************************************/
_SeqIndexMetric::_SeqIndexMetric()
		: mTrackId(0), mModeFlags(0), mGroups(0), mModeW(0)
{
}

_SeqIndexMetric::_SeqIndexMetric(const _SeqIndexMetric& o)
		: mTrackId(o.mTrackId), mModeFlags(o.mModeFlags),
		  mGroups(o.mGroups), mFrame(o.mFrame), mModeW(o.mModeW), mLabel(o.mLabel)
{
}

_SeqIndexMetric::_SeqIndexMetric(	track_id trackId, uint32 trackIndex,
									uint32 modeFlags, uint32 groups,
									BRect frame, float modeW)
		: mTrackId(trackId), mModeFlags(modeFlags), mGroups(groups),
		  mFrame(frame), mModeW(modeW)
{
	mLabel << trackIndex + 1;
}

_SeqIndexMetric& _SeqIndexMetric::operator=(const _SeqIndexMetric& o)
{
	mTrackId = o.mTrackId;
	mModeFlags = o.mModeFlags;
	mGroups = o.mGroups;
	mFrame = o.mFrame;
	mModeW = o.mModeW;
	mLabel = o.mLabel;
	return *this;
}

static rgb_color background_color()
{
	return Prefs().Color(AM_ARRANGE_BG_C);
}

static rgb_color highlight_color()
{
	return Prefs().Color(AM_SONG_SELECTION_C);
}

static rgb_color recording_highlight_color()
{
	return Prefs().Color(AM_SONG_RECORD_SELECTION_C);
}

static void draw_background(BView* view, BRect b, rgb_color bgc,
							float borderL, float borderR, float bottomIndent)
{
	view->SetHighColor( bgc );
	view->FillRect( BRect( b.left + borderL - 1, b.top + 10, b.right - borderR, b.bottom - bottomIndent - 3) );
	view->SetLowColor( bgc );
	/* Shade the edges - black outline
	 */
	view->SetHighColor( 0, 0, 0 );
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

void _SeqIndexMetric::DrawOn(	BView* view, BRect clip,
								SeqSongWinPropertiesI& props,
								float fontHeight)
{
	BRect		b(0, mFrame.top, clip.right, mFrame.bottom);
	float		borderL = 1, borderR = 0;	
	float		bottomIndent = 5;
	float		stringIndent = 4;
	float		bot = mFrame.top + fontHeight + 2 + bottomIndent + stringIndent;
	if (bot < mFrame.bottom) b.bottom = bot;
	rgb_color	bgc = background_color();
	/* Drawn this way just to get pixel-perfect:  The very right edge of
	 * the view should always be drawn with the background color, regardless
	 * of whether or not the track is selected, because a couple pixels
	 * show through behind the mode buttons.
	 */
	if (props.Selections() && props.Selections()->IncludesTrack(mTrackId) ) {
		if (props.IsRecording() ) bgc = recording_highlight_color();
		else bgc = highlight_color();
		draw_background(view, b, bgc, borderL, borderR, bottomIndent);
		draw_background(view, BRect(b.right - 4, b.top, b.right, b.bottom), background_color(), borderL, borderR, bottomIndent);
	} else draw_background(view, b, bgc, borderL, borderR, bottomIndent);
	/* Clean up the cap
	 */
	float		absBottom = b.bottom - bottomIndent + 2;
	view->SetHighColor(0, 0, 0);
	view->StrokeLine( BPoint(0, b.top + 3), BPoint(0, absBottom) );
	/* Cap off if I'm larger then normal.
	 */
	float		fullBottom = mFrame.bottom - bottomIndent + 2;
	if (fullBottom > absBottom && clip.right >= mFrame.right) {
		view->StrokeLine(BPoint(mFrame.right, absBottom), BPoint(mFrame.right, fullBottom));
	}

	/* Draw the label
	 */
	view->SetLowColor(bgc);
	view->SetHighColor( Prefs().Color(AM_ARRANGE_FG_C) );
	if (mLabel.Length() > 0)
		view->DrawString(mLabel.String(), BPoint( b.left + borderL + 2, b.bottom - bottomIndent - stringIndent) );
	/* Draw the group indicator.
	 */
	if (mGroups > 0) {
		const char*	gname = group_name_from(mGroups);
		if (gname) {
			BPoint		mutePt = MuteRect().LeftTop();
			float		w = view->StringWidth("G9");
			view->DrawString(gname, BPoint(mutePt.x - w - 1, b.bottom - bottomIndent - stringIndent));
//			view->FillRect(BRect(b.left + borderL + 2, mFrame.top, b.left + borderL + 8, mFrame.bottom));
		}
	}
	/* Draw the mode buttons.
	 */
	if (mModeW > 0 && clip.right >= mFrame.right - mModeW) {
		BPoint		mutePt = MuteRect().LeftTop(),
					soloPt = SoloRect().LeftTop();

		drawing_mode	mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

		if (mModeFlags&AmTrack::MUTE_MODE && gMuteOn) view->DrawBitmapAsync(gMuteOn, mutePt);
		else if (gMuteOff) view->DrawBitmapAsync(gMuteOff, mutePt);
		if (mModeFlags&AmTrack::SOLO_MODE && gSoloOn) view->DrawBitmapAsync(gSoloOn, soloPt);
		else if (gSoloOff) view->DrawBitmapAsync(gSoloOff, soloPt);

		view->SetDrawingMode(mode);
	}
}

uint32 _SeqIndexMetric::ModeForPoint(BPoint pt) const
{
	if (MuteRect().Contains(pt) ) return AmTrack::MUTE_MODE;
	if (SoloRect().Contains(pt) ) return AmTrack::SOLO_MODE;
	return 0;
}

BRect _SeqIndexMetric::MuteRect() const
{
	return BRect(mFrame.right - mModeW, mFrame.top + 1, mFrame.right - mModeW + 9, mFrame.top + 10);
}

BRect _SeqIndexMetric::SoloRect() const
{
	return BRect(mFrame.right - mModeW, mFrame.top + 11, mFrame.right - mModeW + 9, mFrame.top + 20);
}

void _SeqIndexMetric::SetIndex(uint32 index)
{
//	mIndex = index;
	mLabel = (const char*)NULL;
	mLabel << index + 1;
}

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
static const char* group_name_from(uint32 groups)
{
	if (groups == 0x00000001) return "G1";
	else if (groups == 0x00000002) return "G2";
	else if (groups == 0x00000004) return "G3";
	else if (groups == 0x00000008) return "G4";
	else if (groups == 0x00000010) return "G5";
	else if (groups == 0x00000020) return "G6";
	else if (groups == 0x00000040) return "G7";
	else if (groups == 0x00000080) return "G8";
	else if (groups == 0x00000100) return "G9";
	else return NULL;
}
