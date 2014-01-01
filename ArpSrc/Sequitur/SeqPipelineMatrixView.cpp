/* SeqPipelineMatrixView.cpp
 */
#include <algo.h>
#include <stdio.h>
#include <be/InterfaceKit.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpMultiScrollBar.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "AmPublic/AmPipelineMatrixRef.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqFilterPropertyWindows.h"
#include "Sequitur/SeqPipelineMatrixView.h"
#include "Sequitur/SequiturDefs.h"

static const BBitmap*	gPhraseBg				= NULL;
static const BBitmap*	gPropertyMenuBm			= NULL;
static const BBitmap*	gFilterConnectionBm		= NULL;
static const BBitmap*	gFilterBorderOver		= NULL;
static const BBitmap*	L_BRACKET_PROP_IMAGE	= NULL;

static const float		DRAG_LEFT_INSERT		= 0.20;
/* Each filter slot width is composed like this:
 *	L_BRACKET + BORDER + filter image width + BORDER + R_BRACKET
 */
static const float		L_BRACKET				= 0;
static const float		R_BRACKET				= 9;
static const float		BORDER					= 1;
static const float		FILTER_W				= 20;
static const float		FILTER_H				= 20;
static const float		COL_W					= L_BRACKET + BORDER + FILTER_W + BORDER + R_BRACKET;
static const float		PIPELINE_H				= 5;

static const uint32		CHANGE_FILTER_MSG		= 'iChf';
static const uint32		APPLY_TO_TRACK_MSG		= 'iAtT';
static const uint32		REMOVE_FILTER_MSG		= 'iRmf';
static const uint32		BYPASS_FILTER_MSG		= 'iByf';
static const uint32		FILTER_PROPERTIES_MSG	= 'iFpr';
static const uint32		CONNECT_FILTER_MSG		= 'iCnF';
static const uint32		DISCONNECT_FILTER_MSG	= 'iDcF';
static const uint32		HIDE_PROPERTIES_MSG		= 'oHpF';

/* The pipe layout flags.
 */
static const uint32		OUT_UP					= 0x00000001;
static const uint32		OUT_DOWN				= 0x00000002;
static const uint32		IN_UP					= 0x00000004;
static const uint32		IN_DOWN					= 0x00000008;
static const uint32		THROUGH_UP				= 0x00000010;
static const uint32		THROUGH_DOWN			= 0x00000020;
static const uint32		IN_UP_CLEANUP			= 0x00001000;
static const uint32		IN_DOWN_CLEANUP			= 0x00002000;
static const uint32		LOCK_HRZ_ON				= 0x01000000;
static const uint32		LOCK_HRZ_OFF			= 0x02000000;
static const uint32		CAP_LEFT				= 0x10000000;

static BPopUpMenu*	new_property_menu(	AmPipelineType pipelineType,
										_SeqFilterCell* cell,
										BHandler* target,
										BMessage* archivedFilter,
										bool showPropertyToggle);
static BMenuItem*	new_change_to_menu(	AmPipelineType pipelineType,
										_SeqFilterCell* cell,
										BHandler* target,
										BMessage* archivedFilter);
static BMenuItem*	new_disconnect_item(_SeqFilterCell* cell, BHandler* target);
static bool			accept_for_change_to(AmFilterAddOnHandle* addon, AmPipelineType pipelineType);
static bool			accept_for_change_to(AmFilterAddOn* addon, AmPipelineType pipelineType);
static bool			sort_items(BMenuItem* item1, BMenuItem* item2);

static void			draw_vrt_pipe(BView* view, BRect frame, float pipelineTop, uint32 flag, float pipeShade);
static void			draw_cleanup_pipe(BView* view, float left, float pipelineTop, uint32 flags, float pipeShade);

/*************************************************************************
 * _SEQ-FILTER-GRID
 * This class stores all the cells that make up the pipeline matrix, along
 * with all necessary info to draw it.
 *************************************************************************/
class _SeqFilterCell;
class _SeqFilterLoc;
class _LayoutCell;

class _SeqFilterGrid
{
public:
	_SeqFilterGrid(	const AmPipelineMatrixI* matrix,
					AmPipelineType type,
					uint32 growBy = 3);
	virtual ~_SeqFilterGrid();

	status_t	FillCells(const AmPipelineMatrixI* matrix);
	status_t	ValidateConnection(_SeqFilterCell* fromCell, _SeqFilterCell* toCell) const;
	void		DrawOn(BView* view, BRect clip, rgb_color viewColor);

	_SeqFilterCell*		CellAt(pipeline_id pid, filter_id fid);
	_SeqFilterCell*		CellAt(uint32 row, uint32 col) const;
	_SeqFilterCell*		CellAt(BPoint where);
	_SeqFilterCell*		CellAt(_SeqFilterLoc* location) const;
	_SeqFilterCell*		NextValidCell(uint32 row, uint32 col);

	BRect				FrameFor(_SeqFilterLoc* location) const;
	BRect				IconFrameFor(_SeqFilterLoc* location) const;
	int32				FilterIndexOnOrAfter(uint32 row, uint32 col) const;
	void				GetSize(float* width, float* height) const;
	
	void				Print() const;

private:
	_SeqFilterCell**	mCells;
	AmPipelineType		mPipelineType;
	uint32				mRows, mCols;
	uint32				mPipelineCount;
	uint32				mGrowBy;
	float				mPipeShade;
	
	status_t			GrowTo(uint32 rows, uint32 cols);
	void				FreeCells();

	bool				ConnectionExists(_SeqFilterCell* fromCell, _SeqFilterCell* toCell) const;
	/* The layout algorithm.
	 */
	status_t			NewLayout(const AmPipelineMatrixI* matrix);
	bool				ShiftCells(_LayoutCell** cells, uint32 rows, uint32 cols) const;
	void				LayoutPipes(uint32 row, uint32 col);
	void				CacheLayoutCell(_LayoutCell& cell, uint32 row, int32 filterIndex,
										_LayoutCell** cells, uint32 rows, uint32 cols);
	/* This is all just setup for the algorithm.
	 */
	_LayoutCell**		GetLayoutCells(	const AmPipelineMatrixI* matrix,
												uint32* w, uint32* h) const;
	void				MakeLayoutConnections(	const AmPipelineMatrixI* matrix,
												_LayoutCell** cells, uint32 rows, uint32 cols,
												_LayoutCell* cell) const;
	void				PushBackConnection(	_LayoutCell* cell,
											AmFilterHolderI* h,
											_LayoutCell** cells, uint32 rows, uint32 cols) const;
	void				FreeLayoutCells(_LayoutCell** cells, uint32 rows, uint32 cols) const;
	void				PrintLayoutCells(_LayoutCell** cells, uint32 rows, uint32 cols) const;
};

/*************************************************************************
 * _SEQ-FILTER-CELL
 *************************************************************************/
class _SeqFilterInfo;

class _SeqFilterCell
{
public:
	_SeqFilterCell(	uint32 row, uint32 col,
					float top, float bottom);
	virtual ~_SeqFilterCell();
	
	void			Initialize(	uint32 row, uint32 col,
								float top, float bottom);
	
	uint32			Row() const;
	uint32			Col() const;
	uint32			Flags() const;
	void			SetFlag(uint32 flag, bool on);
	void			SetFlags(uint32 flags);
	float			Top() const;
	float			Bottom() const;
	BRect			Frame() const;
	BRect			IconFrame() const;
	void			SetMetrics(float top, float bottom);

	_SeqFilterInfo*	FilterInfo();
	void			SetFilterInfo(_SeqFilterInfo* info);
	status_t		CacheFilter(pipeline_id pid, AmFilterHolderI* holder,
								int32 filterIndex);
	void			SwapFilterInfo(_SeqFilterCell& cell);
	/* Convenience accessing of the filter info.
	 */
	pipeline_id		PipelineId() const;
	filter_id		FilterId() const;
	int32			FilterIndex() const;
	bool			IsBypassed() const;
	bool			HasToolTip() const;
	const char*		ToolTip() const;
	uint32			FilterFlags() const;
	
	bool			HitPropertyMenu(BPoint where) const;
	BPoint			PropertyMenuOrigin() const;
	bool			HitConnection(BPoint where) const;
	BPoint			ConnectionOrigin() const;
	bool			CanMakeConnection() const;
	
	void			DrawOn(	BView* view, BRect clippedFrame, float pipelineTop,
							rgb_color viewColor, float pipeShade);

	AmFilterHolder*	FilterHolder(	const AmPipelineMatrixI* matrix,
									AmPipelineType type) const;

private:
	uint32			mRow, mCol;
	float			mTop, mBottom;
	uint32			mFlags;
	_SeqFilterInfo*	mFilterInfo;
};

/*************************************************************************
 * _SEQ-FILTER-LOC
 *************************************************************************/
class _SeqFilterLoc
{
public:
	_SeqFilterLoc();
	_SeqFilterLoc(const _SeqFilterLoc& o);
	_SeqFilterLoc(pipeline_id pid, filter_id fid);
	_SeqFilterLoc(	pipeline_id pid, filter_id fid,
					const BString& label);
	_SeqFilterLoc(	pipeline_id pid, filter_id fid,
					uint32 row, uint32 col,
					const BString& label);
	_SeqFilterLoc(const _SeqFilterCell& cell);

	_SeqFilterLoc&	operator=(const _SeqFilterLoc& o);
	
	bool			Equals(const _SeqFilterLoc& o) const;
	
	pipeline_id		mPid;
	filter_id		mFid;
	int32			mRow, mCol;

	/* These are little hackish growths that let the drawing
	 * routines know how to display properly when the loc is
	 * being used as the mOverLoc.
	 */
	bool			mNoProperties;
	bool			mNoConnections;
	/* More hackish growths, so the disconnect menu can
	 * give the user a name.
	 */
	BString			mLabel;
};

/*************************************************************************
 * _SEQ-FILTER-INFO
 *************************************************************************/
class _SeqFilterInfo
{
public:
	_SeqFilterInfo(const AmFilterHolderI* holder, int32 filterIndex, uint32 row);
	_SeqFilterInfo(	pipeline_id pipelineId, const AmFilterHolderI* holder,
					int32 filterIndex, uint32 row);
	virtual ~_SeqFilterInfo();
	
	void			DrawOn(BView* view, BRect frame, rgb_color viewColor);
	bool			CanMakeConnection() const;

	vector<_SeqFilterLoc> mConnections;

	pipeline_id		mPipelineId;
	filter_id		mFilterId;
	int32			mFilterIndex;
	bool			mBypassed;
	BString			mToolTip;
	BBitmap*		mIcon;
	int32			mMaxConnections;
	uint32			mFlags;
		
	void			MakeConnections(const AmFilterHolderI* holder);
};

/*************************************************************************
 * SEQ-PIPELINE-MATRIX-VIEW
*************************************************************************/
SeqPipelineMatrixView::SeqPipelineMatrixView(	BRect frame,
												const char* name,
												AmPipelineMatrixRef matrixRef,
												AmPipelineType pipelineType,
												uint32 flags)
		: inherited(frame, name,
					B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
					B_WILL_DRAW | B_FRAME_EVENTS),
		  BToolTipable(*(BView*)this),
		  mPipelineType(pipelineType), mMatrixRef(matrixRef), mGrid(NULL),
		  mHsb(NULL), mDownButtons(0), mOverLoc(NULL), mConnLoc(NULL),
		  mDragLoc(NULL), mDragCode(NO_DRAG),
		  mFlags(flags), mConnectionPt(-1, -1), mShowProperties(false)
{
	if (!gPhraseBg) gPhraseBg = ImageManager().FindBitmap(AM_PHRASE_BG_STR);
	if (!gPropertyMenuBm) gPropertyMenuBm = ImageManager().FindBitmap(SEQ_PROPERTY_MENU_NORMAL_STR);
	if (!gFilterConnectionBm) gFilterConnectionBm = ImageManager().FindBitmap("Filter Connection Over");
	if (!gFilterBorderOver) gFilterBorderOver = ImageManager().FindBitmap(FILTER_BORDER_OVER);
	if (!L_BRACKET_PROP_IMAGE) L_BRACKET_PROP_IMAGE = ImageManager().FindBitmap(SEQ_PROPERTY_MENU_NORMAL_STR);
	mFilterSize = seq_app->FilterImageSize();
}

SeqPipelineMatrixView::~SeqPipelineMatrixView()
{
	delete mGrid;
	delete mOverLoc;
	mOverLoc = NULL;
	delete mConnLoc;
	mConnLoc = NULL;
	delete mDragLoc;
	mDragLoc = NULL;
}

void SeqPipelineMatrixView::SetMatrixRef(AmPipelineMatrixRef matrixRef)
{
	mMatrixRef = matrixRef;
	FillMetrics();
	Invalidate();
}

void SeqPipelineMatrixView::SetShowProperties(bool state)
{
	mShowProperties = state;
}

void SeqPipelineMatrixView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	if (!(mFlags&SEQ_FORCE_VIEW_COLOR) && Parent() ) mViewColor = Parent()->ViewColor();
	mMatrixRef.AddMatrixPipelineObserver(0, this);
	if (mPipelineType == INPUT_PIPELINE || mPipelineType == DESTINATION_PIPELINE)
		mMatrixRef.AddMatrixFilterObserver(0, this);
}

void SeqPipelineMatrixView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mMatrixRef.RemoveMatrixObserver(0, this);
}

void SeqPipelineMatrixView::Draw(BRect clip)
{
	BView* into = this;
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqPipelineMatrixView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	InitializeHsb();
}

void SeqPipelineMatrixView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = 0;
	if (!mGrid) return;
	mGrid->GetSize(width, height);
}

void SeqPipelineMatrixView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes != 1) {
		inherited::KeyDown(bytes, numBytes);
		return;
	}

	switch ( bytes[0] ) {
		case B_DELETE: {
			_SeqFilterCell*		cell;
			if (mGrid && mOverLoc && (cell = mGrid->CellAt(mOverLoc)) != NULL && cell->FilterInfo() != NULL) {
				RemoveFilter(cell->PipelineId(), cell->FilterId() );
			}
		} break;
		inherited::KeyDown(bytes, numBytes);
			break;
	}
}

void SeqPipelineMatrixView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case AmNotifier::PIPELINE_CHANGE_OBS: {
			FillMetrics();
			Invalidate();
		} break;
		case AmNotifier::FILTER_CHANGE_OBS: {
			FillMetrics();
			Invalidate();
		} break;
		case AmSong::TRACK_CHANGE_OBS:
			TrackChangeReceived(msg);
			break;
		case ARPMSG_FILTERADDONDRAG: {
			BMessage addon;
			for (int32 i=0;
					msg->FindMessage(SZ_FILTER_ARCHIVE, i, &addon) == B_OK;
					i++) {
				if (Accept(&addon) ) {
					HandleAddOnDrop(msg, &addon);
					break;
				}
			}
		} break;
		case CONNECT_FILTER_MSG: {
			pipeline_matrix_id	mid;
			pipeline_id			pid;
			filter_id			fid;
			BPoint				where;
			uint32				button;
			GetMouse(&where, &button, false);
			if (msg->FindPointer(MATRIX_ID_STR, &mid) == B_OK && mid == mMatrixRef.Id()
					&& msg->FindPointer("pipeline_id", &pid) == B_OK
					&& msg->FindPointer("filter_id", &fid) == B_OK) {
				HandleConnectionDrop(pid, fid, where);
			}
		} break;
		case DISCONNECT_FILTER_MSG: {
			pipeline_id		opid, tpid;
			filter_id		ofid, tfid;
			if (mGrid && msg->FindPointer("owner_pipeline_id", &opid) == B_OK
					&& msg->FindPointer("owner_filter_id", &ofid) == B_OK
					&& msg->FindPointer("target_pipeline_id", &tpid) == B_OK
					&& msg->FindPointer("target_filter_id", &tfid) == B_OK) {
				// WRITE MATRIX BLOCK
				AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
				if (matrix) {
					matrix->BreakConnection(opid, mPipelineType, ofid, tpid, mPipelineType, tfid);
					if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
				}
				mMatrixRef.WriteUnlock(matrix);
				// END WRITE MATRIX BLOCK
			}
		} break;
		case BYPASS_FILTER_MSG: {
			pipeline_id			pid;
			filter_id			fid;
			if (msg->FindPointer("pipeline_id", &pid) == B_OK
					&& msg->FindPointer("filter_id", &fid) == B_OK)
				SetBypassed(pid, fid);
		} break;
		case HIDE_PROPERTIES_MSG: {
			pipeline_id			pid;
			filter_id			fid;
			if (msg->FindPointer("pipeline_id", &pid) == B_OK
					&& msg->FindPointer("filter_id", &fid) == B_OK)
				SetHideProperties(pid, fid);
		} break;
		case CHANGE_FILTER_MSG:
			HandleChangeToMsg(msg);
			break;
		case FILTER_PROPERTIES_MSG:
			HandleFilterPropertiesMsg(msg);
			break;
		case APPLY_TO_TRACK_MSG: {
			pipeline_id		pid;
			filter_id		fid;
			if (msg->FindPointer("pipeline_id", &pid) == B_OK
					&& msg->FindPointer("filter_id", &fid) == B_OK)
				ApplyToTrack(pid, fid);
		} break;
		case REMOVE_FILTER_MSG: {
			pipeline_id		pid;
			filter_id		fid;
			if (msg->FindPointer("pipeline_id", &pid) == B_OK
					&& msg->FindPointer("filter_id", &fid) == B_OK) {
				RemoveFilter(pid, fid);
			}
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void SeqPipelineMatrixView::MouseDown(BPoint where)
{
	mDownButtons = 0;
	mDownPoint.Set(-1, -1);
	mConnectionPt.Set(-1, -1);
	delete mConnLoc;
	mConnLoc = NULL;
	
	_SeqFilterCell*		cell = (mGrid) ? mGrid->CellAt(where) : NULL;
	if (!cell) return;

	if (cell->HitPropertyMenu(where) ) {
		BMessage			archive;
		status_t			err = B_ERROR;
		// READ MATRIX BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqPipelineMatrixView::MouseDown() read lock 1\n"); fflush(stdout);
		#endif
		const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
		if (matrix) {
			AmFilterHolder*			holder = cell->FilterHolder(matrix, mPipelineType);
			if (holder && holder->Filter() ) err = holder->Filter()->Archive(&archive, 0);
		}
		mMatrixRef.ReadUnlock(matrix);
		// END READ MATRIX BLOCK
		HandlePropertyMenu(cell, where, (err == B_OK) ? &archive : NULL);
		return;
	}

	if (cell->HitConnection(where)) {
		BMessage		cnnMsg(CONNECT_FILTER_MSG);
		cnnMsg.AddPointer(MATRIX_ID_STR, mMatrixRef.Id() );
		cnnMsg.AddPointer("pipeline_id", cell->PipelineId());
		cnnMsg.AddPointer("filter_id", cell->FilterId());
		mDownPoint = where;
		if (gFilterConnectionBm) {
			BBitmap*	bmp = new BBitmap(gFilterConnectionBm);
			BPoint		pt(bmp->Bounds().Width() / 2, bmp->Bounds().Height() / 2);
			DragMessage(&cnnMsg, bmp, B_OP_BLEND, pt, Window() );
		} else {
			DragMessage(&cnnMsg, BRect(0, 0, 10, 10), Window() );
		}
		return;
	}

	if (cell->FilterInfo() == NULL) return;

	if (Window()->CurrentMessage()->FindInt32("buttons", &mDownButtons) != B_OK)
		mDownButtons = 0;

	int32 clicks = 1;
	Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	if (mDownButtons == B_PRIMARY_MOUSE_BUTTON ||
			mDownButtons == B_SECONDARY_MOUSE_BUTTON) {
		// This can either initiate a drag or double click to open
		// configuration panel.
		if (clicks == 2) {
			mDownButtons = 0;
			AmFilterHolder* filter = NULL;
			BMessenger		win;
			
			// READ MATRIX BLOCK
			#ifdef AM_TRACE_LOCKS
			printf("SeqPipelineMatrixView::MouseDown() read lock 2\n"); fflush(stdout);
			#endif
			const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
			if (matrix) {
				filter = cell->FilterHolder(matrix, mPipelineType);
				if (filter) {
					filter->IncRefs();
					win = filter->ConfigWindow();
				}
			}
			mMatrixRef.ReadUnlock(matrix);
			// END READ MATRIX BLOCK
			
			if (win.IsValid() ) {
				BMessage msg(SEQ_ACTIVATE);
				win.SendMessage(&msg);
				if (filter) filter->DecRefs();
				
			} else if (filter) {
				SeqFilterPropertyWindow* cw = new SeqFilterPropertyWindow(mMatrixRef, mPipelineType, filter, Window() );
				if (cw) {
					cw->Show();
					// WRITE MATRIX BLOCK
					#ifdef AM_TRACE_LOCKS
					printf("SeqPipelineMatrixView::MouseUp() write lock\n"); fflush(stdout);
					#endif
					AmPipelineMatrixI*	matrix = mMatrixRef.WriteLock();
					if (matrix) filter->SetConfigWindow(BMessenger(cw));
					mMatrixRef.WriteUnlock(matrix);
					// END WRITE MATRIX BLOCK
				}
				filter->DecRefs();
			}
		} else {
			SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
			mDownPoint = where;
		}
	/* Let the filter handle middle mouse button clicks.  The right
	 * button is reserved for drags with copy / move menus.
	 */
	} else if (mDownButtons == B_TERTIARY_MOUSE_BUTTON) {
		mDownButtons = 0;
		mDownPoint = where;
		BRect				frame;
		AmFilterHolderI*	filter = NULL;
		// READ MATRIX BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqPipelineMatrixView::MouseUp() read lock 3\n"); fflush(stdout);
		#endif
		const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
		if (matrix) {
			filter = cell->FilterHolder(matrix, mPipelineType);
			if (filter) filter->IncRefs();
		}
		mMatrixRef.ReadUnlock(matrix);
		// END READ MATRIX BLOCK
		if (filter) {
			filter->Filter()->MouseAction(	ConvertToScreen(frame), ConvertToScreen(where),
											mDownButtons, false);
			filter->DecRefs();
		}
	}
}

void SeqPipelineMatrixView::MouseMoved(	BPoint where,
										uint32 code,
										const BMessage* dragMessage)
{
	inherited::MouseMoved(where, code, dragMessage);
	BPoint		oldConnectionPt = mConnectionPt;
	mConnectionPt.Set(-1, -1);
	if (!mGrid) return;
	
	BRect		b = Bounds();
	if (!b.Contains(where) ) {
		BRect		r = arp_invalid_rect();
		if (mOverLoc) {
			r = arp_merge_rects(mGrid->IconFrameFor(mOverLoc), r);
			delete mOverLoc;
			mOverLoc = NULL;
		}
		if (mConnLoc) {
			r = arp_merge_rects(mGrid->IconFrameFor(mConnLoc), r);
			delete mConnLoc;
			mConnLoc = NULL;
		}
		if (mDragLoc) {
			r = arp_merge_rects(mGrid->IconFrameFor(mDragLoc), r);
			delete mDragLoc;
			mDragLoc = NULL;
		}
		if (arp_is_valid_rect(r) ) Invalidate(r);
		return;
	}

	if (!IsFocus() ) MakeFocus(true);
	
	int32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	if (!dragMessage && buttons && mDownButtons) {
		// This is a drag, see if we should initiate a filter drag.
		float delta = fabsf(mDownPoint.x-where.x) + fabsf(mDownPoint.y-where.y);
		if (delta > 3) {
			if (CanDrag() ) {
				_SeqFilterCell*	cell = mGrid->CellAt(where);
				if (cell && cell->FilterInfo() ) {
					// WRITE MATRIX BLOCK
					AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
					if (matrix) StartAddOnDrag(matrix, cell, buttons);
					mMatrixRef.WriteUnlock(matrix);
					// END WRITE MATRIX BLOCK
				}
			}
			mDownButtons = 0;
		}
		return;
	}
	
	if (!Bounds().Contains(where) ) return;
	
	BRect		r = arp_invalid_rect();
	/* HANDLE DRAGGING AN ADDON
	 */
	if (dragMessage && dragMessage->what == ARPMSG_FILTERADDONDRAG) {
		BMessage addon;
		for (int32 i=0;
				dragMessage->FindMessage(SZ_FILTER_ARCHIVE, i, &addon) == B_OK;
				i++) {
			if (Accept(&addon) ) {
				HandleAddOnDrag(where);
				break;
			}
		}
	/* HANDLE DRAGGING A CONNECTION
	 */
	} else if (dragMessage && dragMessage->what == CONNECT_FILTER_MSG) {
		float		l = mDownPoint.x, t = mDownPoint.y, rt = mDownPoint.x, b = mDownPoint.y;
		if (oldConnectionPt.x >= 0 && oldConnectionPt.y >= 0) {
			if (oldConnectionPt.x < l) l = oldConnectionPt.x;
			if (oldConnectionPt.y < t) t = oldConnectionPt.y;
			if (oldConnectionPt.x > rt) rt = oldConnectionPt.x;
			if (oldConnectionPt.y > b) b = oldConnectionPt.y;
		}
		mConnectionPt = where;
		BRect	r2(	min(l, where.x), min(t, where.y),
					max(rt, where.x), max(b, where.y) );
		r2.left -= 3;  r2.top -= 3;  r2.right += 3;  r2.bottom += 3;
		r = arp_merge_rects(r, r2);
		/* Find the filter at the current location.  If it valid for a drop,
		 * highlight it.
		 */
		if (mConnLoc) {
			r = arp_merge_rects(mGrid->IconFrameFor(mConnLoc), r);
			delete mConnLoc;
			mConnLoc = NULL;
		}
		_SeqFilterCell*		fromCell = mGrid->CellAt(mDownPoint);
		_SeqFilterCell*		toCell = mGrid->CellAt(mConnectionPt);
		if (fromCell && toCell
				&& mGrid->ValidateConnection(fromCell, toCell) == B_OK) {
			mConnLoc = new _SeqFilterLoc(*toCell);
			if (mConnLoc) {
				mConnLoc->mNoProperties = true;
				mConnLoc->mNoConnections = true;
				r = arp_merge_rects(mGrid->IconFrameFor(mConnLoc), r);
			}
		}
	/* HANDLE EVERYTHING ELSE
	 */
	} else {
		/* Find the _SeqFilterCell at the current mouse location and set it as
		 * the active border.  Basically, if there are any differences at
		 * all between the current filter slot and the mOverLoc, then
		 * invalidate them both.
		 */
		_SeqFilterLoc*		loc = NULL;
		_SeqFilterCell*		cell = mGrid->CellAt(where);
		/* If the mouse is pressed, then I shouldn't have a mouse-over effect.
		 */
		if (buttons == 0 && cell) loc = new _SeqFilterLoc(*cell);
		if (mOverLoc && loc) {
			if (mOverLoc->Equals(*loc) ) {
				delete loc;
				loc = NULL;
			} else {
				r = arp_merge_rects(mGrid->IconFrameFor(mOverLoc), mGrid->IconFrameFor(loc));
				delete mOverLoc;
				mOverLoc = NULL;
				mOverLoc = loc;
			}
		} else if (mOverLoc && !loc) {
			r = arp_merge_rects(mGrid->IconFrameFor(mOverLoc), r);
			delete mOverLoc;
			mOverLoc = NULL;
		} else if (!mOverLoc && loc) {
			r = arp_merge_rects(mGrid->IconFrameFor(loc), r);
			mOverLoc = loc;
		}
	}

	if (arp_is_valid_rect(r) ) Invalidate(r);
}

void SeqPipelineMatrixView::MouseUp(BPoint where)
{
	inherited::MouseUp(where);

	if (mDownButtons != 0 && mDownButtons != B_PRIMARY_MOUSE_BUTTON) {
		_SeqFilterCell*		cell = (mGrid) ? mGrid->CellAt(where) : NULL;
		if (cell && cell->FilterInfo() ) {
			AmFilterHolderI*	filter = NULL;
			// READ MATRIX BLOCK
			#ifdef AM_TRACE_LOCKS
			printf("SeqPipelineMatrixView::MouseUp() read lock\n"); fflush(stdout);
			#endif
			const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
			if (matrix) {
				filter = cell->FilterHolder(matrix, mPipelineType);
				if (filter) filter->IncRefs();
			}
			mMatrixRef.ReadUnlock(matrix);
			// END READ MATRIX BLOCK
			if (filter) {
				filter->Filter()->MouseAction(	ConvertToScreen(cell->IconFrame() ), ConvertToScreen(where),
												mDownButtons, true);
				filter->DecRefs();
			}
		}
	}

	mDownButtons = 0;
	ClearMouseOver();
}

void SeqPipelineMatrixView::SetHorizontalScrollBar(BScrollBar* sb)
{
	mHsb = sb;
	if (mHsb) mHsb->SetTarget(this);
}

void SeqPipelineMatrixView::InitializeScrollBars()
{
	InitializeHsb();
}

void SeqPipelineMatrixView::ClearMouseOver()
{
	if (!mGrid) return;
	BRect		r = arp_invalid_rect();
	if (mOverLoc) {
		r = arp_merge_rects(mGrid->IconFrameFor(mOverLoc), r);
		delete mOverLoc;
		mOverLoc = NULL;
	}
	if (mConnLoc) {
		r = arp_merge_rects(mGrid->IconFrameFor(mConnLoc), r);
		delete mConnLoc;
		mConnLoc = NULL;
	}
	if (arp_is_valid_rect(r) ) Invalidate(r);
}

void SeqPipelineMatrixView::ForceViewColor(rgb_color c)
{
	mViewColor = c;
	mFlags |= SEQ_FORCE_VIEW_COLOR;
}

status_t SeqPipelineMatrixView::GetToolTipInfo(	BPoint where, BRect* out_region,
												BToolTipInfo* out_info)
{
	if (!mGrid) return B_ERROR;
	_SeqFilterCell*		cell = mGrid->CellAt(where);
	if (!cell || !cell->HasToolTip() ) return B_ERROR;
	*out_region = cell->IconFrame();
	if (out_info) out_info->SetText(cell->ToolTip() );
	return B_OK;
}

void SeqPipelineMatrixView::FillMetrics()
{
	// READ MATRIX BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPipelineMatrixView::FillMetrics() read lock\n"); fflush(stdout);
	#endif
	const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
	if (matrix) FillMetrics(matrix);
	mMatrixRef.ReadUnlock(matrix);
	// END READ MATRIX BLOCK
}

void SeqPipelineMatrixView::FillMetrics(const AmPipelineMatrixI* matrix)
{
	ArpASSERT(matrix);
	if (mGrid) delete mGrid;
	mGrid = new _SeqFilterGrid(matrix, mPipelineType);
}

void SeqPipelineMatrixView::RemoveFilter(pipeline_id pid, filter_id fid)
{
	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		matrix->RemoveFilter(pid, mPipelineType, fid);
		if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
}

void SeqPipelineMatrixView::SetBypassed(pipeline_id pid, filter_id fid)
{
	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		AmFilterHolderI*	holder = matrix->Filter(pid, mPipelineType, fid);
		if (holder) {
			if (holder->IsBypassed() ) holder->SetBypassed(false);
			else holder->SetBypassed(true);
			matrix->PipelineChanged(pid, mPipelineType);
		}
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
}

void SeqPipelineMatrixView::SetHideProperties(pipeline_id pid, filter_id fid)
{
	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		AmFilterHolderI*	holder = matrix->Filter(pid, mPipelineType, fid);
		if (holder && holder->Filter() ) {
			AmFilterI*		f = holder->Filter();
			if (f->Flags()&f->HIDE_PROPERTIES_FLAG) f->SetFlag(f->HIDE_PROPERTIES_FLAG, false);
			else f->SetFlag(f->HIDE_PROPERTIES_FLAG, true);
			matrix->PipelineChanged(pid, mPipelineType);
		}
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
}

bool SeqPipelineMatrixView::Accept(const BMessage* addon) const
{
	int32 tmp;
	if (addon->FindInt32(SZ_FILTER_TYPE, &tmp) != B_OK) return false;
	return Accept( (AmFilterAddOn::type)tmp );
}

bool SeqPipelineMatrixView::Accept(AmFilterAddOn::type type) const
{
	if (mPipelineType == INPUT_PIPELINE) {
		if (type == AmFilterAddOn::DESTINATION_FILTER) return false;
		return true;
	}
	if (mPipelineType == THROUGH_PIPELINE) {
		if (type != AmFilterAddOn::THROUGH_FILTER) return false;
		return true;
	}
	if (mPipelineType == OUTPUT_PIPELINE) {
		if (type != AmFilterAddOn::THROUGH_FILTER) return false;
		return true;
	}
	if (mPipelineType == DESTINATION_PIPELINE) {
		return type == AmFilterAddOn::DESTINATION_FILTER;
	}
	if (mPipelineType == NULLINPUTOUTPUT_PIPELINE) return true;
	return false;
}

bool SeqPipelineMatrixView::CanDrag() const
{
	return true;
}

static inline void draw_over_loc(	BView* view, _SeqFilterLoc* loc,
									_SeqFilterGrid* grid)
{
	BRect				f(grid->IconFrameFor(loc));
	view->DrawBitmapAsync(gFilterBorderOver, f.LeftTop() );
	if (gPropertyMenuBm && !loc->mNoProperties)
		view->DrawBitmapAsync(gPropertyMenuBm, f.LeftTop() );
	_SeqFilterCell*		cell = (grid) ? grid->CellAt(loc) : NULL;
	if (gFilterConnectionBm && cell && cell->CanMakeConnection()
			&& !loc->mNoConnections) {
		view->DrawBitmapAsync(gFilterConnectionBm, cell->ConnectionOrigin() );
	}
}

void SeqPipelineMatrixView::DrawOn(BView* view, BRect clip)
{
	if (gPhraseBg && !mFlags&SEQ_SUPPRESS_BACKGROUND) arp_tile_bitmap_on(view, clip, gPhraseBg, BPoint(0,0) );
	else {
		view->SetHighColor(mViewColor);
		view->FillRect(clip);
	}

	if (!mGrid) FillMetrics();
	if (!mGrid) return;
	mGrid->DrawOn(view, clip, mViewColor);

	// Draw the Black Line of Connection
#if 0
	if (mConnectionPt.x >= 0 && mConnectionPt.y >= 0) {
		drawing_mode		mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->SetHighColor(0, 0, 0, 127);
		float				penSize = view->PenSize();
		view->SetPenSize(2);
		view->StrokeLine(mDownPoint, mConnectionPt);
		view->SetPenSize(penSize);
		view->SetDrawingMode(mode);	
	}
#endif
	drawing_mode		mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	if (mDragLoc) {
		view->SetHighColor(0, 0, 0, 127);
		BRect		f(mGrid->IconFrameFor(mDragLoc));
		if (mDragCode == DRAG_LEFT)
			f.right = f.left + (f.Width() * DRAG_LEFT_INSERT);
		else if (mDragCode == DRAG_CENTER) {
			f.left = f.left + L_BRACKET;
			f.right = f.left + BORDER + FILTER_W + BORDER - 1;
		} else if (mDragCode == DRAG_RIGHT)
			f.left = f.right - R_BRACKET;
		view->FillRect(f);
		view->SetDrawingMode(mode);	
	} else {
		if (mOverLoc) draw_over_loc(view, mOverLoc, mGrid);
		if (mConnLoc) draw_over_loc(view, mConnLoc, mGrid);
	}

	view->SetDrawingMode(mode);	
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

void SeqPipelineMatrixView::TrackChangeReceived(BMessage* msg)
{
	FillMetrics();

	BRect		bounds = Bounds();
	int32		position;
	status_t	err = lowest_position(*msg, &position);
	if (err == B_OK && position >= 0) {
		// READ MATRIX BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqPipelineMatrixView::TrackChangeReceived() read lock\n"); fflush(stdout);
		#endif
		const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
		if (matrix) {
			float	top = 0;
			float	bottom = -1;
			for (uint32 k = 0; k <= uint32(position); k++) {
				float		height;
				if (matrix->PipelineHeight(k, &height) == B_OK) {
					top = bottom + 1;
					bottom = top + height;
				}
			}
			bounds.top = top;
		}
		mMatrixRef.ReadUnlock(matrix);
		// END READ MATRIX BLOCK
	}

	Invalidate(bounds);
}

void SeqPipelineMatrixView::InitializeHsb()
{
	if (!mHsb) return;
	SetHsbRange();
	SetHsbSteps();
}

void SeqPipelineMatrixView::SetHsbRange()
{
	float 	width = HorizontalWidth();

	if (width <= Frame().Width()) width = 0;
	else width = width - Frame().Width();
	mHsb->SetRange(0, width);
}

void SeqPipelineMatrixView::SetHsbSteps()
{
	float	min, max;
	BRect	bounds = Bounds();
	float 	width = HorizontalWidth();

	mHsb->GetRange(&min, &max);
	if ((min == 0) && (max == 0)) return;

	float	bigStep = bounds.Width();
	float	smallStep = 10;

	if (bigStep > 20) bigStep -= smallStep;
	if (bigStep > width - bounds.Width()) bigStep = width - bounds.Width();
	if ((bounds.right + smallStep) > width) smallStep = width - bounds.right;
	mHsb->SetSteps(smallStep, bigStep);
	
	float	prop = bounds.Width() / width;
	if( prop > 1 ) prop = 1;
	if( prop < 0 ) prop = 0;
	mHsb->SetProportion(prop);
}

float SeqPipelineMatrixView::HorizontalWidth() const
{
	if (!mGrid) return 0;
	float		w, h;
	mGrid->GetSize(&w, &h);
	return w;
}

void SeqPipelineMatrixView::HandlePropertyMenu(	_SeqFilterCell* cell,
												BPoint where,
												BMessage* archivedFilter)
{
	ArpVALIDATE(cell, return);
	/* If there's a mouse-over image, clear it out here, because lord
	 * knows where the mouse is going to end up when it's done with
	 * the menu.
	 * FIX:  Of course, it doesn't actually work -- the menu must be
	 * locking the window before it has a chance to draw.  I suppose
	 * I could just call Draw() directly.
	 */
	if (mOverLoc) {
		BRect				r = arp_invalid_rect();
		r = arp_merge_rects(r, mGrid->IconFrameFor(mOverLoc) );
		delete mOverLoc;
		mOverLoc = NULL;
		if (arp_is_valid_rect(r) ) Invalidate(r);
	}

	AmPipelineType		tmpType = mPipelineType;
	if (mPipelineType == INPUT_PIPELINE && cell->FilterIndex() == 0) tmpType = SOURCE_PIPELINE;
	BPopUpMenu*			menu = new_property_menu(tmpType, cell, this, archivedFilter, mShowProperties);
	if (!menu) return;
	menu->SetTargetForItems(this);
	menu->SetAsyncAutoDestruct(true);
	where = ConvertToScreen(where);
	BRect				sticky(where.x-5, where.y-5, where.x+5, where.y+5);
	menu->Go(where, true, false, sticky, true);
}

void SeqPipelineMatrixView::StartAddOnDrag(	const AmPipelineMatrixI* matrix,
											_SeqFilterCell* cell,
											int32 buttons)
{
	ArpVALIDATE(matrix && cell, return);
	AmFilterHolder*		holder = cell->FilterHolder(matrix, mPipelineType);
	ArpASSERT(holder && holder->Filter() );
	if (!holder || !holder->Filter() ) return;
	const AmFilterI*	filter = holder->Filter();
	BBitmap*			image = ArpMakeFilterBitmap(filter, BPoint(20, 20), cell->Row() % 10);
	
	BMessage	archive;
	status_t	err = filter->Archive(&archive, 0);
	
	BMessage	msg;
	msg.what = ARPMSG_FILTERADDONDRAG;
	if (err == B_OK) err = msg.AddMessage(SZ_FILTER_ARCHIVE, &archive);
	if (err == B_OK) err = msg.AddPointer("filter_id", filter->Id() );
	if (err == B_OK) err = msg.AddPointer("pipeline_id", cell->PipelineId() );
	if (err == B_OK) err = msg.AddPointer(MATRIX_ID_STR, matrix->Id() );
	if (err == B_OK) err = msg.AddInt32("pipeline_type", (int32)mPipelineType);
	if (err == B_OK) err = msg.AddInt32("buttons", buttons);

	if (err != B_OK) delete image;
	else {
		if (!image) DragMessage(&msg, BRect(0, 0, 20, 20) );
		else {
			BPoint		pt(image->Bounds().Width() / 2, image->Bounds().Height() / 2);
			DragMessage(&msg, image, B_OP_BLEND, pt);
		}
	}
}

void SeqPipelineMatrixView::HandleAddOnDrag(BPoint where)
{
	if (!mGrid) return;
	BRect				r = arp_invalid_rect();
	_SeqFilterLoc*		loc = NULL;
	_SeqFilterCell*		cell = mGrid->CellAt(where);
	if (cell) loc = new _SeqFilterLoc(*cell);
	if (mDragLoc && loc) {
		if (mDragLoc->Equals(*loc) ) {
			delete loc;
			loc = NULL;
		} else {
			r = arp_merge_rects(mGrid->IconFrameFor(mDragLoc), mGrid->IconFrameFor(loc));
			delete mDragLoc;
			mDragLoc = loc;
		}
	} else if (mDragLoc && !loc) {
		r = arp_merge_rects(mGrid->IconFrameFor(mDragLoc), r);
		delete mDragLoc;
		mDragLoc = NULL;
		mDragCode = NO_DRAG;
	} else if (!mDragLoc && loc) {
		r = arp_merge_rects(mGrid->IconFrameFor(loc), r);
		mDragLoc = loc;
	}
	
	if (mDragLoc) {
		BRect			f(mGrid->IconFrameFor(mDragLoc));
		uint32			lastDragCode = mDragCode;
		if (where.x <= (f.left + (f.Width() * DRAG_LEFT_INSERT))) mDragCode = DRAG_LEFT;
		else if (where.x >= f.right - R_BRACKET) mDragCode = DRAG_RIGHT;
		else mDragCode = DRAG_CENTER;
		if (lastDragCode != mDragCode) r = arp_merge_rects(r, f);
	}
	if (arp_is_valid_rect(r) ) Invalidate(r);
}

void SeqPipelineMatrixView::HandleAddOnDrop(const BMessage* msg, const BMessage* addon)
{
	if (!mDragLoc || !mGrid) return;

	bool		move = false;
	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		move = LockedHandleAddOnDrop(matrix, msg, addon);
		/* If this is a move operation, and the filter is being moved to a track
		 * in the same song, then perform that here so the move happens within a
		 * single write lock.
		 */
		if (move) {
			pipeline_matrix_id			matrixId;
			if( (msg->FindPointer(MATRIX_ID_STR, &matrixId ) == B_OK)
					&& matrixId == matrix->Id() ) {
				move = false;
				pipeline_id		pipelineId;
				int32			pipelineType;
				filter_id		filterId;
				if ( (msg->FindPointer("pipeline_id", &pipelineId) == B_OK)
						&& (msg->FindPointer("filter_id", &filterId) == B_OK)
						&& (msg->FindInt32("pipeline_type", &pipelineType) == B_OK) ) {
					matrix->RemoveFilter(pipelineId, (AmPipelineType)pipelineType, filterId);
				}
			}
		}
		if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK

	/* If the filter is being moved, attempt to remove it from its original owner.
	 */
	if (move) {
		pipeline_matrix_id matrixId;
		pipeline_id		pipelineId;
		int32			pipelineType;
		filter_id		filterId;
		if ( (msg->FindPointer(MATRIX_ID_STR, &matrixId) == B_OK)
				&& (msg->FindPointer("pipeline_id", &pipelineId) == B_OK)
				&& (msg->FindPointer("filter_id", &filterId) == B_OK)
				&& (msg->FindInt32("pipeline_type", &pipelineType) == B_OK) ) {
			AmPipelineMatrixRef		matrixRef = AmGlobals().PipelineMatrixRef(matrixId);
			if (matrixRef.IsValid() ) {
				// WRITE MATRIX BLOCK
				matrix = matrixRef.WriteLock();
				if (matrix) {
					matrix->RemoveFilter(pipelineId, (AmPipelineType)pipelineType, filterId);
					if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
				}
				matrixRef.WriteUnlock(matrix);
				// END WRITE MATRIX BLOCK
			}
		}
	}

	BRect		r = arp_invalid_rect();
	if (mDragLoc) {
		r = arp_merge_rects(r, mGrid->IconFrameFor(mDragLoc) );
		delete mDragLoc;
		mDragLoc = NULL;
		mDragCode = NO_DRAG;
	}
	if (arp_is_valid_rect(r) ) Invalidate(r);
}

bool SeqPipelineMatrixView::LockedHandleAddOnDrop(	AmPipelineMatrixI* matrix,
													const BMessage* msg,
													const BMessage* archive)
{
	if (!mGrid) return false;

	bool				replace = false;
	int32				filterIndex = -1;
	pipeline_id			pid = mDragLoc->mPid;
	if (pid == 0) pid = matrix->PipelineId(mDragLoc->mRow);
	ArpVALIDATE(pid != 0, return false);
	if (mDragCode == DRAG_LEFT)
		filterIndex = mGrid->FilterIndexOnOrAfter(mDragLoc->mRow, mDragLoc->mCol);
	else if (mDragCode == DRAG_CENTER) {
		filterIndex = mGrid->FilterIndexOnOrAfter(mDragLoc->mRow, mDragLoc->mCol);
		if (mDragLoc->mFid != 0) replace = true;
	} else if (mDragCode == DRAG_RIGHT)
		filterIndex = mGrid->FilterIndexOnOrAfter(mDragLoc->mRow, mDragLoc->mCol + 1);
	else return false;

	ArpRef<AmFilterAddOn> addon = am_find_filter_addon(archive);
	if (!addon) return false;
	
	/* This is the move or copy flag.  By default I copy, but if it gets set to
	 * true, that means I should move the filter that's being dropped, so at the
	 * end of this method I'll reply to the message with the secret handshake to
	 * remove it.
	 */
	bool		move = false;
	filter_id	id = 0;
	BPoint		where;
	ulong		buttons;
	GetMouse(&where, &buttons, false);
	if (msg->FindPointer("filter_id", &id) == B_OK) {
		int32	dragButtons;
		move = true;
		if (msg->FindInt32("buttons", &dragButtons) == B_OK) {
			if (dragButtons&B_SECONDARY_MOUSE_BUTTON) {
				uint32	op = DroppedMenu(where);
				if (op == 2) return false;
				if (op == 1) move = false;
			}
		}
	}


	if (replace) matrix->ReplaceFilter(addon, pid, mPipelineType, filterIndex, archive);
	else matrix->InsertFilter(addon, pid, mPipelineType, filterIndex, archive);

	return move;
}

uint32 SeqPipelineMatrixView::DroppedMenu(BPoint where) const
{
	BPopUpMenu*		menu = new BPopUpMenu("menu");
	if (!menu) return 2;
	menu->SetFontSize(10);
	BMenuItem*		move = new BMenuItem("Move Here", new BMessage('move') );
	BMenuItem*		copy = new BMenuItem("Copy Here", new BMessage('copy') );
	BMenuItem*		cancel = new BMenuItem("Cancel", new BMessage('cncl') );
	if (!move || !copy || !cancel) {
		delete menu;
		delete move;
		delete copy;
		delete cancel;
		return 2;
	}
	menu->AddItem(move);
	menu->AddItem(copy);
	menu->AddSeparatorItem();
	menu->AddItem(cancel);

	uint32			retVal = 2;
	BMenuItem*		answer = menu->Go( ConvertToScreen(where), false, true, false );
	if (answer && answer->Message() ) {
		if (answer->Message()->what == 'move') retVal = 0;
		else if (answer->Message()->what == 'copy') retVal = 1;
	}
	delete menu;
	return retVal;
}

void SeqPipelineMatrixView::HandleConnectionDrop(pipeline_id pid, filter_id fid, BPoint where)
{
	if (!mGrid) return;
	_SeqFilterCell*		fromCell = mGrid->CellAt(pid, fid);
	_SeqFilterCell*		toCell = mGrid->CellAt(where);
	if (!fromCell || !toCell) return;

	if (mGrid->ValidateConnection(fromCell, toCell) != B_OK) return;

	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		matrix->MakeConnection(pid, mPipelineType, fid, toCell->PipelineId(), mPipelineType, toCell->FilterId() );
		if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK

#if 0
	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		AmFilterHolder*			from = dynamic_cast<AmFilterHolder*>(matrix->Filter(pid, mPipelineType, fid));
		AmFilterHolder*			to = dynamic_cast<AmFilterHolder*>(matrix->Filter(toCell->PipelineId(), mPipelineType, toCell->FilterId()));
		if (from && to && from != to && from->Filter() && from->Filter()->AddOn() ) {
			/* This is a special rule -- normally, all holders automatically have
			 * their NextInLine() holder as a connection.  However, in the case of
			 * filters with only 1 allowed connection, they can redirect that
			 * connection.
			 */
			if (from->Filter()->AddOn()->MaxConnections() == 1) {
				AmFilterHolder*		h;
				while ((h = from->RemoveConnection(int32(0))) != NULL) ;
				from->SetSuppressNextInLine(true);
			}
			from->AddConnection(to);
			fromCell->FilterInfo()->MakeConnections(from);
			matrix->PipelineChanged(pid, mPipelineType);
		}
		if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
#endif




#if 0
printf("CONNECTIONS:\n");
uint32		c = matrix->CountPipelines();
for (uint32 k = 0; k < c; k++) {
	AmFilterHolderI*	filt = matrix->Filter(matrix->PipelineId(k), OUTPUT_PIPELINE);
	if (filt) {
		printf("\tPIPELINE %ld\n", k);
		while (filt) {
			printf("\t\t%s connections:\n", filt->Filter()->Name() );
			uint32		c2 = filt->CountConnections();
			for (uint32 k2 = 0; k2 < c2; k2++) {
				AmFilterHolderI*	f = filt->ConnectionAt(k2);
				if (f) printf("\t\t\t%s\n", f->Filter()->Name() );
			}
			filt = filt->NextInLine();
		}
	}
}
#endif
}

void SeqPipelineMatrixView::HandleChangeToMsg(const BMessage* msg)
{
	if (!mGrid) return;
	int32				row, col;
	if (msg->FindInt32("cell_row", &row) != B_OK
			|| msg->FindInt32("cell_col", &col) != B_OK)
		return;
	_SeqFilterCell*		cell = mGrid->CellAt(row, col);
	if (!cell) return;
	int32				index = -1;
	pipeline_id			pid = 0;
	if (cell->FilterInfo() ) {
		pid = cell->PipelineId();
		index = cell->FilterIndex();		
	} else {
		/* Find the first filter to my right, so I can insert in front
		 * of it.
		 */
		_SeqFilterCell*	rightCell = mGrid->NextValidCell(row, col + 1);
		if (rightCell && rightCell->FilterInfo() )
			index = rightCell->FilterIndex();
	}

	BMessage			archive;
	msg->FindMessage(SZ_FILTER_ARCHIVE, &archive);
	ArpRef<AmFilterAddOn> addon = am_find_filter_addon(&archive);
	if (!addon) return;

	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*	matrix = mMatrixRef.WriteLock();
	if (matrix) {
		if (!pid) {
			pid = matrix->PipelineId(row);
			if (pid) matrix->InsertFilter(addon, pid, mPipelineType, index, &archive);
		} else if (index == -1) matrix->InsertFilter(addon, pid, mPipelineType, -1, &archive);
		else matrix->ReplaceFilter(addon, pid, mPipelineType, index, &archive);
		if (matrix->UndoContext() ) matrix->UndoContext()->CommitState();
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
}

void SeqPipelineMatrixView::HandleFilterPropertiesMsg(const BMessage* msg)
{
	filter_id			fid;
	if (msg->FindPointer("filter_id", &fid) != B_OK) return;
	pipeline_id			pid;
	if (msg->FindPointer("pipeline_id", &pid) != B_OK) return;
	AmFilterHolder*		holder = NULL;
	// WRITE MATRIX BLOCK
	AmPipelineMatrixI*	matrix = mMatrixRef.WriteLock();
	if (matrix) {
		holder = dynamic_cast<AmFilterHolder*>(matrix->Filter(pid, mPipelineType, fid) );
		if (holder) holder->IncRefs();
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
	if (!holder) return;
	BMessenger		win = holder->ConfigWindow();
	if (win.IsValid() ) {
		BMessage	msg(SEQ_ACTIVATE);
		win.SendMessage(&msg);
	} else if (holder->Filter() ) {
		SeqFilterPropertyWindow* cw = new SeqFilterPropertyWindow(mMatrixRef, mPipelineType, holder, Window() );
		if (cw) {
			cw->Show();
			/* Get a second write lock because I don't want the matrix to be locked
			 * when the property window opens -- right now it would actually be OK,
			 * but it's possible this would be a bad thing at some point in the future.
			 */
			// WRITE MATRIX BLOCK
			AmPipelineMatrixI*	matrix = mMatrixRef.WriteLock();
			if (matrix) holder->SetConfigWindow(BMessenger(cw));
			mMatrixRef.WriteUnlock(matrix);
			// END WRITE MATRIX BLOCK
		}
	}
	holder->DecRefs();
}


BRect SeqPipelineMatrixView::BoundsForPipeline(int32 pipelineIndex) const
{
	// READ MATRIX BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPipelineMatrixView::BoundsForPipeline() read lock\n"); fflush(stdout);
	#endif
	BRect		b(Bounds() );
	b.top = b.bottom = 0;
	const AmPipelineMatrixI*	matrix = mMatrixRef.ReadLock();
	if (matrix) {
		for (int32 k = 0; k <= pipelineIndex; k++) {
			if (b.top != 0) b.top = b.bottom + 1;
			float		h;
			if (matrix->PipelineHeight(uint32(pipelineIndex), &h) == B_OK)
				b.bottom = b.top + h;			
		}
	}
	mMatrixRef.ReadUnlock(matrix);
	// END READ MATRIX BLOCK
	return b;
}

float SeqPipelineMatrixView::LeftBracket() const
{
	return L_BRACKET;
}

float SeqPipelineMatrixView::RightBracket() const
{
	return R_BRACKET;
}

float SeqPipelineMatrixView::Border() const
{
	return BORDER;
}

// #pragma mark -

/*************************************************************************
 * SEQ-SONG-PIPELINE-MATRIX-VIEW
 *************************************************************************/
SeqSongPipelineMatrixView::SeqSongPipelineMatrixView(	BRect frame,
														const char* name,
														AmPipelineMatrixRef matrixRef,
														AmPipelineType pipelineType,
														AmSongRef songRef)
		: inherited(frame, name, matrixRef, pipelineType),
		  mSongRef(songRef)
{
}

void SeqSongPipelineMatrixView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	mSongRef.AddObserver(this, AmSong::TRACK_CHANGE_OBS);
}

void SeqSongPipelineMatrixView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mSongRef.RemoveObserverAll(this);
}

void SeqSongPipelineMatrixView::ApplyToTrack(pipeline_id pid, filter_id fid)
{
	// WRITE SONG BLOCK
	AmSong*			song = mSongRef.WriteLock();
	if (song) {
		AmTrack*	track = song ? song->Track(pid) : 0;
		if (track) {
			HandleApplyToTrackMsg(track, fid);
			track->RemoveFilter(mPipelineType, fid);
			if ( song->UndoContext() ) {
				song->UndoContext()->SetUndoName("Apply Filter to Track");
				song->UndoContext()->CommitState();
			}
		}
	}
	mSongRef.WriteUnlock(song);
	// END SONG MATRIX BLOCK
}

void SeqSongPipelineMatrixView::HandleApplyToTrackMsg(AmTrack* track, filter_id id)
{
	AmFilterHolderI*	holder = track->Filter(mPipelineType, id);
	if (!holder || !holder->Filter() ) return;
	AmNode*				node = track->Phrases().HeadNode();
	AmNode*				nextNode;
	while (node) {
		nextNode = node->next;
		AmPhraseEvent*		topPhrase = dynamic_cast<AmPhraseEvent*>(node->Event() );
		HandleApplyToTrackMsg(track, holder->Filter(), topPhrase, node->Event(), NULL);
		node = nextNode;
	}
}

void SeqSongPipelineMatrixView::HandleApplyToTrackMsg(	AmTrack* track, AmFilterI* filter,
														AmPhraseEvent* topPhrase, AmEvent* event,
														AmPhrase* parent)
{
	ArpASSERT(track && filter && event);
	if (event->Type() == event->PHRASE_TYPE) {
		AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(event);
		if (!pe) return;
		/* I can't just iterate over the events -- if the filter's HandleEvent()
		 * method generates new events then I might get put into an infinite loop.
		 * I need some way of caching the original state of the phrase -- I use
		 * a selection object for this.
		 */
		AmSelectionsI*	selections = AmSelectionsI::NewSelections();
		if (!selections) return;
		if (selections->AddPhrase(track->Id(), topPhrase, pe) != B_OK) {
			delete selections;
			return;
		}
		track_id		tid;
		AmEvent*		e;
		AmPhraseEvent*	fakePe;
		for (uint32 k = 0; selections->EventAt(0, k, &tid, &fakePe, &e) == B_OK; k++)
			HandleApplyToTrackMsg(track, filter, topPhrase, e, pe->Phrase() );
		if ( pe->IsEmpty() ) track->RemoveEvent(parent, event);
		delete selections;
		return;
	}

	AmEvent*		gen = event->Copy();
	if (!gen) return;
	track->RemoveEvent(parent, event);

	am_filter_params params;
	AmTime			endTime = ( track->Song() ) ? track->Song()->CountEndTime() : 0;
	AmEvent*		tempos = ( track->Song() ) ? track->Song()->PlaybackList(0, endTime, PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_SIGNATURE)
												   : NULL;
	AmEvent*		signatures = ( track->Song() ) ? track->Song()->PlaybackList(0, endTime, PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_TEMPO)
												   : NULL;
	if (tempos) tempos = tempos->HeadEvent();
	if (signatures) signatures = signatures->HeadEvent();
	params.cur_tempo = dynamic_cast<AmTempoChange*>(tempos);
	params.cur_signature = dynamic_cast<AmSignature*>(signatures);

	gen = filter->HandleEvent(gen, &params);
	while (gen) {
		track->AddEvent(parent, gen);
		gen = gen->RemoveEvent();
	}
}

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
static BPopUpMenu* new_property_menu(	AmPipelineType pipelineType,
										_SeqFilterCell* cell,
										BHandler* target,
										BMessage* archivedFilter,
										bool showPropertyToggle)
{
	ArpASSERT(cell && target);
	BPopUpMenu*			menu = new BPopUpMenu("properties menu");
	if (!menu) return NULL;
	BMessage*			msg;
	BMenuItem*			item;
	pipeline_id			pid = cell->PipelineId();
	filter_id			fid = cell->FilterId();

	/* The Change To menu item.
	 */
	AmPipelineType		rtype = pipelineType;
	if (rtype == INPUT_PIPELINE && cell->Col() == 0) rtype = SOURCE_PIPELINE;
	if ( (item = new_change_to_menu(rtype, cell, target, archivedFilter)) )
		menu->AddItem(item);
	/* The Disconnect menu item.
	 */
	if ( (item = new_disconnect_item(cell, target)) )
		menu->AddItem(item);
	/* The Remove menu item.
	 */
	msg = new BMessage(REMOVE_FILTER_MSG);
	if (msg && (item = new BMenuItem("Remove", msg)) ) {
		if (pid) msg->AddPointer("pipeline_id", pid);
		if (fid) msg->AddPointer("filter_id", fid);
		else item->SetEnabled(false);
		menu->AddItem(item);
	}
	/* The Apply to Track menu item.
	 */
	msg = new BMessage(APPLY_TO_TRACK_MSG);
	if (msg && (item = new BMenuItem("Apply to Track", msg)) ) {
		if (pid) msg->AddPointer("pipeline_id", pid);
		if (fid && pipelineType == OUTPUT_PIPELINE) msg->AddPointer("filter_id", fid);
		else item->SetEnabled(false);
		menu->AddItem(item);
	}	
	/* The Bypass menu item.
	 */
	menu->AddSeparatorItem();
	msg = new BMessage(BYPASS_FILTER_MSG);
	if (msg && (item = new BMenuItem("Bypass", msg)) ) {
		if (pid) msg->AddPointer("pipeline_id", pid);
		if (fid) msg->AddPointer("filter_id", fid);
		else item->SetEnabled(false);
		if (cell->IsBypassed() ) item->SetMarked(true);
		menu->AddItem(item);
	}
	/* The Properties menu item.
	 */
	menu->AddSeparatorItem();
	msg = new BMessage(FILTER_PROPERTIES_MSG);
	if (msg && (item = new BMenuItem("Properties", msg)) ) {
		if (pid) msg->AddPointer("pipeline_id", pid);
		if (fid) msg->AddPointer("filter_id", fid);
		else item->SetEnabled(false );
		menu->AddItem(item);
	}
	if (showPropertyToggle) {
		msg = new BMessage(HIDE_PROPERTIES_MSG);
		if (msg && (item = new BMenuItem("Hide Properties", msg)) ) {
			if (pid) msg->AddPointer("pipeline_id", pid);
			if (fid) msg->AddPointer("filter_id", fid);
			else item->SetEnabled(false);
			if (cell->FilterFlags()&AmFilterI::HIDE_PROPERTIES_FLAG) item->SetMarked(true);
			menu->AddItem(item);
		}
	}
		
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont(be_plain_font);
	return menu;
}

static BMenuItem* new_change_to_menu(	AmPipelineType pipelineType,
										_SeqFilterCell* cell,
										BHandler* target,
										BMessage* archivedFilter)
{
	AmFilterRoster*		roster = AmFilterRoster::Default();
	BMenu*				menu = new BMenu("Change To");
	if (!menu) return NULL;
	vector<BMenuItem*>		items;
	
	roster->Locker()->Lock();
	const int32				N = roster->CountAddOns();
	for (int32 k = 0; k < N; k++) {
		AmFilterAddOnHandle* handle = dynamic_cast<AmFilterAddOnHandle*>(roster->AddOnAt(k));
		if (handle && accept_for_change_to(handle, pipelineType) ) {
			BString			label;
			if (handle->AddOn() ) handle->AddOn()->GetLabel(label, true, archivedFilter);
			else label = handle->Name();
			BMessage*		msg = new BMessage;
			BMenuItem*		item = NULL;
			if (msg && label.Length() > 0 && (item = new BMenuItem(label.String(), msg)) ) {
				BMessage	archive;
				status_t	err = handle->GetArchiveTemplate(&archive);
				if (err == B_OK) err = msg->AddMessage(SZ_FILTER_ARCHIVE, &archive);
				if (err == B_OK) err = msg->AddInt32("cell_row", cell->Row() );
				if (err == B_OK) err = msg->AddInt32("cell_col", cell->Col() );
				if (err == B_OK) {
					msg->what = CHANGE_FILTER_MSG;
					items.push_back(item);
				} else {
					delete item;
				}
			}
		}
	}
	roster->Locker()->Unlock();
	
	AmMultiFilterRoster*		roster2 = AmMultiFilterRoster::Default();
	if (roster2) {
		ArpRef<AmMultiFilterAddOn>	addon = NULL;
		for (uint32 k = 0; (addon = roster2->FilterAt(k)) != NULL; k++) {
			if (accept_for_change_to(addon, pipelineType)) {
				BString			label;
				addon->GetLabel(label, true, archivedFilter);
				BMessage*		msg = new BMessage;
				BMenuItem*		item = NULL;
				if (msg && label.Length() > 0 && (item = new BMenuItem(label.String(), msg)) ) {
					BMessage	archive;
					status_t	err = addon->GetArchiveTemplate(&archive, AM_ARCHIVE_ALL);
					if (err == B_OK) err = msg->AddMessage(SZ_FILTER_ARCHIVE, &archive);
					if (err == B_OK) err = msg->AddInt32("cell_row", cell->Row() );
					if (err == B_OK) err = msg->AddInt32("cell_col", cell->Col() );
					if (err == B_OK) {
						msg->what = CHANGE_FILTER_MSG;
						items.push_back(item);
					} else {
						delete item;
					}
				}
			}
		}
	}
	
	sort(items.begin(), items.end(), sort_items);
	for (uint32 k = 0; k < items.size(); k++) menu->AddItem(items[k]);
	items.resize(0);
	
	BMenuItem*		item = new BMenuItem(menu);
	if (!item) {
		delete menu;
		return NULL;
	}
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont(be_plain_font);
	menu->SetTargetForItems(target);
	return item;
}

static BMenuItem* new_disconnect_item(_SeqFilterCell* cell, BHandler* target)
{
	BMenu*				menu = new BMenu("Disconnect");
	if (!menu) return NULL;
	BMenuItem*			item = NULL;
	_SeqFilterInfo*		info = cell->FilterInfo();
	bool				hasItem = false;
	if (info) {
		for (uint32 k = 0; k < info->mConnections.size(); k++) {
			BString			label;
			label << k + 1;
			if (info->mConnections[k].mLabel.Length() > 0)
				label << " " << info->mConnections[k].mLabel;
			BMessage*		msg = new BMessage(DISCONNECT_FILTER_MSG);
			BMenuItem*		item = new BMenuItem(label.String(), msg, 0, 0);
			if (msg && item) {
				msg->AddPointer("owner_pipeline_id", cell->PipelineId() );
				msg->AddPointer("owner_filter_id", cell->FilterId() );
				msg->AddPointer("target_pipeline_id", info->mConnections[k].mPid);
				msg->AddPointer("target_filter_id", info->mConnections[k].mFid);
				menu->AddItem(item);
				hasItem = true;
			}
		}
	}
	if (!hasItem) {
		delete menu;
		item = new BMenuItem("Disconnect", new BMessage(), 0, 0);
		if (item) item->SetEnabled(false);
		return item;
	}
	item = new BMenuItem(menu);
	if (!item) {
		delete menu;
		return NULL;
	}
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont(be_plain_font);
	menu->SetTargetForItems(target);
	return item;
}

static bool accept_for_change_to(AmFilterAddOnHandle* addon, AmPipelineType pipelineType)
{
	ArpVALIDATE(addon, return false);
	if (pipelineType == NULLINPUTOUTPUT_PIPELINE) {
		if (addon->Type() == AmFilterAddOn::THROUGH_FILTER) return true;
		if (addon->Key() == NULL_INPUT_KEY) return true;
		if (addon->Key() == NULL_OUTPUT_KEY) return true;
		return false;
	}
	if (pipelineType == SOURCE_PIPELINE || addon->Type() == AmFilterAddOn::SOURCE_FILTER) {
		if (addon->Type() != AmFilterAddOn::SOURCE_FILTER || !(pipelineType == SOURCE_PIPELINE) ) return false;
		else return true;
	}
	if (pipelineType == DESTINATION_PIPELINE || addon->Type() == AmFilterAddOn::DESTINATION_FILTER) {
		if (addon->Type() != AmFilterAddOn::DESTINATION_FILTER || !(pipelineType == DESTINATION_PIPELINE) ) return false;
		else return true;
	}
	return true;
}

static bool accept_for_change_to(AmFilterAddOn* addon, AmPipelineType pipelineType)
{
	ArpVALIDATE(addon, return false);
	if (pipelineType == NULLINPUTOUTPUT_PIPELINE) {
		if (addon->Type() == addon->THROUGH_FILTER) return true;
		if (strcmp(addon->Key().String(), NULL_INPUT_KEY) == 0) return true;
		if (strcmp(addon->Key().String(), NULL_OUTPUT_KEY) == 0) return true;
		return false;
	}
	if (pipelineType == SOURCE_PIPELINE || addon->Type() == AmFilterAddOn::SOURCE_FILTER) {
		if (addon->Type() != AmFilterAddOn::SOURCE_FILTER || !(pipelineType == SOURCE_PIPELINE) ) return false;
		else return true;
	}
	if (pipelineType == DESTINATION_PIPELINE || addon->Type() == AmFilterAddOn::DESTINATION_FILTER) {
		if (addon->Type() != AmFilterAddOn::DESTINATION_FILTER || !(pipelineType == DESTINATION_PIPELINE) ) return false;
		else return true;
	}
	return true;
}

static bool sort_items(BMenuItem* item1, BMenuItem* item2)
{
	if (item1->Label() == NULL) return false;
	if (item2->Label() == NULL) return true;
	BString		str(item1->Label() );
	if (str.ICompare(item2->Label() ) <= 0) return true;
	return false;
}

static void draw_vrt_pipe(BView* view, BRect f, float pipeShade)
{
	rgb_color		c = Prefs().Color(AM_PIPELINE_C);
	view->SetHighColor(c);
	view->StrokeLine( BPoint(f.left + 1, f.top),	BPoint(f.left + 1, f.bottom) );
	view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
	view->StrokeLine( BPoint(f.left + 2, f.top),	BPoint(f.left + 2, f.bottom) );
	view->SetHighColor( seq_darken(c, PIPELINE_SHADE_2 * pipeShade) );
	view->StrokeLine( BPoint(f.left + 3, f.top),	BPoint(f.left + 3, f.bottom) );
	view->SetHighColor( seq_darken(c, PIPELINE_SHADE_3 * pipeShade) );
	view->StrokeLine( BPoint(f.left, f.top),		BPoint(f.left, f.bottom) );
	view->StrokeLine( BPoint(f.left + 4, f.top),	BPoint(f.left + 4, f.bottom) );
}

static void draw_vrt_pipe(BView* view, BRect frame, float pipelineTop, uint32 flag, float pipeShade)
{
	rgb_color		c = Prefs().Color(AM_PIPELINE_C);
	if (flag&THROUGH_UP) {
		BRect		f(frame.left + 2, frame.top, frame.left + 2 + 4, frame.bottom);
		f.bottom = pipelineTop - 1;
		draw_vrt_pipe(view, f, pipeShade);
	}
	if (flag&THROUGH_DOWN) {
		BRect		f(frame.left + 2, frame.top, frame.left + 2 + 4, frame.bottom);
		f.top = pipelineTop + PIPELINE_H;
		draw_vrt_pipe(view, f, pipeShade);
	}
	if (flag&LOCK_HRZ_OFF && (flag&THROUGH_UP || flag&THROUGH_DOWN) ) {
		BRect		f(frame.left + 2, pipelineTop, frame.left + 2 + 4, pipelineTop + PIPELINE_H);
		draw_vrt_pipe(view, f, pipeShade);
	}
	if (flag&OUT_UP) {
		BRect		f(frame.left + 2, frame.top, frame.left + 2 + 4, frame.bottom);
		f.bottom = pipelineTop - 6;
		draw_vrt_pipe(view, f, pipeShade);
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_3 * pipeShade) );
		view->StrokeLine(BPoint(frame.left, pipelineTop - 4), BPoint(f.left, f.bottom));
		view->StrokeLine(BPoint(frame.left, pipelineTop), BPoint(f.right, f.bottom));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_2 * pipeShade) );
		view->StrokeLine(BPoint(frame.left, pipelineTop - 1), BPoint(f.right - 1, f.bottom));
		view->SetHighColor(c);
		view->StrokeLine(BPoint(frame.left, pipelineTop - 2), BPoint(f.right - 2, f.bottom));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
		view->StrokeLine(BPoint(frame.left, pipelineTop - 3), BPoint(f.right - 3, f.bottom));
	}
	if (flag&OUT_DOWN) {
		BRect		f(frame.left + 2, frame.top, frame.left + 2 + 4, frame.bottom);
		f.top = pipelineTop + PIPELINE_H + 5;
		draw_vrt_pipe(view, f, pipeShade);
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_3 * pipeShade) );
		view->StrokeLine(BPoint(frame.left, pipelineTop + PIPELINE_H - 1), BPoint(f.right, f.top));
		view->StrokeLine(BPoint(frame.left, f.top - 2), BPoint(frame.left + 2, f.top));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_2 * pipeShade) );
		view->StrokeLine(BPoint(frame.left, pipelineTop + PIPELINE_H), BPoint(frame.left, f.top - 3));
		view->StrokeLine(BPoint(frame.left + 1, f.top - 2), BPoint(frame.left + 2, f.top - 1));
		view->SetHighColor(c);
		view->StrokeLine(BPoint(f.left, f.top - 2), BPoint(f.left + 1, f.top - 1));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
		view->StrokeLine(BPoint(frame.left + 1, pipelineTop + PIPELINE_H + 1), BPoint(f.right - 1, f.top));
		view->StrokeLine(BPoint(frame.left + 1, pipelineTop + PIPELINE_H + 2), BPoint(frame.left + 1, pipelineTop + PIPELINE_H + 2));
	}
	if (flag&IN_UP) {
		BRect		f(frame.left + 2, frame.top, frame.left + 2 + 4, frame.bottom);
		f.bottom = pipelineTop - 6;
		draw_vrt_pipe(view, f, pipeShade);
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_3 * pipeShade) );
		view->StrokeLine(BPoint(f.left, f.bottom), BPoint(frame.right, pipelineTop));
		view->StrokeLine(BPoint(f.right, f.bottom), BPoint(frame.right, pipelineTop - 4));
		view->SetHighColor(c);
		view->StrokeLine(BPoint(f.left + 2, f.bottom), BPoint(frame.right, pipelineTop - 2));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
		view->StrokeLine(BPoint(f.left + 1, f.bottom), BPoint(frame.right, pipelineTop - 1));
		view->StrokeLine(BPoint(f.left + 3, f.bottom), BPoint(frame.right, pipelineTop - 3));
	}
	if (flag&IN_DOWN) {
		BRect		f(frame.left + 2, frame.top, frame.left + 2 + 4, frame.bottom);
		f.top = pipelineTop + PIPELINE_H + 5;
		draw_vrt_pipe(view, f, pipeShade);
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_3 * pipeShade) );
		view->StrokeLine(BPoint(f.left, f.top), BPoint(frame.right, pipelineTop + PIPELINE_H - 1));
		view->StrokeLine(BPoint(f.right, f.top), BPoint(frame.right, pipelineTop + PIPELINE_H + 3));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_2 * pipeShade) );
		view->StrokeLine(BPoint(f.right - 1, f.top), BPoint(frame.right, pipelineTop + PIPELINE_H + 2));
		view->SetHighColor(c);
		view->StrokeLine(BPoint(f.right - 2, f.top), BPoint(frame.right, pipelineTop + PIPELINE_H + 1));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
		view->StrokeLine(BPoint(f.right - 3, f.top), BPoint(frame.right, pipelineTop + PIPELINE_H));
	}
}

static void draw_cleanup_pipe(BView* view, float left, float pipelineTop, uint32 flags, float pipeShade)
{
	rgb_color		c = Prefs().Color(AM_PIPELINE_C);
	if (flags&IN_DOWN_CLEANUP) {
		view->SetHighColor(0, 0, 0);
		view->StrokeLine(BPoint(left, pipelineTop + PIPELINE_H + 2), BPoint(left + 2, pipelineTop + PIPELINE_H));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_2 * pipeShade) );
		view->StrokeLine(BPoint(left, pipelineTop + PIPELINE_H + 1), BPoint(left + 2, pipelineTop + PIPELINE_H - 1));
		view->SetHighColor(c);
		view->StrokeLine(BPoint(left, pipelineTop + PIPELINE_H), BPoint(left + 2, pipelineTop + PIPELINE_H - 2));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
		view->StrokeLine(BPoint(left, pipelineTop + PIPELINE_H - 1), BPoint(left + 1, pipelineTop + PIPELINE_H - 2));
	}
	if (flags&IN_UP_CLEANUP) {
		view->SetHighColor(0, 0, 0);
		view->StrokeLine(BPoint(left, pipelineTop - 3), BPoint(left + 2, pipelineTop - 1));
		view->SetHighColor( seq_darken(c, PIPELINE_SHADE_1 * pipeShade) );
		view->StrokeLine(BPoint(left, pipelineTop - 2), BPoint(left + 2, pipelineTop));
		view->StrokeLine(BPoint(left, pipelineTop), BPoint(left, pipelineTop));
		view->SetHighColor(c);
		view->StrokeLine(BPoint(left, pipelineTop - 1), BPoint(left + 2, pipelineTop + 1));		
	}
}

// #pragma mark -

/*************************************************************************
 * _SEQ-FILTER-GRID
 *************************************************************************/
_SeqFilterGrid::_SeqFilterGrid(	const AmPipelineMatrixI* matrix,
								AmPipelineType type, uint32 growBy)
		: mCells(NULL), mPipelineType(type), mRows(0), mCols(0),
		  mPipelineCount(0), mGrowBy(growBy), mPipeShade(0.85)
{
	FillCells(matrix);
}

_SeqFilterGrid::~_SeqFilterGrid()
{
	FreeCells();
}

static bool accept_addon_type(AmPipelineType pipelineType, AmFilterAddOn::type addonType)
{
	if (pipelineType == INPUT_PIPELINE) {
		if (addonType == AmFilterAddOn::DESTINATION_FILTER) return false;
	} else if (pipelineType == THROUGH_PIPELINE) {
		if (addonType == AmFilterAddOn::SOURCE_FILTER) return false;
		if (addonType == AmFilterAddOn::DESTINATION_FILTER) return false;
	} else if (pipelineType == OUTPUT_PIPELINE) {
		if (addonType == AmFilterAddOn::SOURCE_FILTER) return false;
		if (addonType == AmFilterAddOn::DESTINATION_FILTER) return false;
	} else if (pipelineType == DESTINATION_PIPELINE) {
		if (addonType != AmFilterAddOn::DESTINATION_FILTER) return false;
	}
	return true;
}

status_t _SeqFilterGrid::FillCells(const AmPipelineMatrixI* matrix)
{
	ArpASSERT(matrix);
	NewLayout(matrix);
	return B_OK;
}

status_t _SeqFilterGrid::ValidateConnection(_SeqFilterCell* fromCell, _SeqFilterCell* toCell) const
{
	ArpASSERT(fromCell && toCell);
	/* Make sure that adding a connection from fromCell to toCell is valid.
	 */
	if (!(fromCell->FilterInfo() ) || !(toCell->FilterInfo() ) ) return B_ERROR;	
	/* From and to can not be in the same pipeline.
	 */
	if (fromCell->PipelineId() == toCell->PipelineId() ) return B_ERROR;
	if (fromCell->Row() == toCell->Row() ) return B_ERROR;
	/* From cell can not have more than one connection to toCell's pipeline.
	 */
	vector<_SeqFilterLoc>&		con = fromCell->FilterInfo()->mConnections;
	for (uint32 k = 0; k < con.size(); k++) {
		if (con[k].mPid == toCell->PipelineId() ) return B_ERROR;
	}
	/* toCell can not end up connecting back to a cell at or before fromCell.
	 */
	if (ConnectionExists(toCell, fromCell) ) return B_ERROR;
	
/* RULE:  If any filters later in your pipe have connections to different
 * pipes, you can not form a connection to a filter later than those connections
 * - i.e. no crossed wires.
 */
	return B_OK;
}

bool _SeqFilterGrid::ConnectionExists(_SeqFilterCell* fromCell, _SeqFilterCell* toCell) const
{
	if (!fromCell || !(fromCell->FilterInfo()) || !toCell || !(toCell->FilterInfo()) )
		return false;
	vector<_SeqFilterLoc>&		con = fromCell->FilterInfo()->mConnections;
	for (uint32 k = 0; k < con.size(); k++) {
		if (con[k].mPid == toCell->PipelineId()
				&& con[k].mFid == toCell->FilterId() )
			return true;
		_SeqFilterCell*		nextCell = CellAt(&con[k]);
		if (nextCell && nextCell->FilterInfo() ) {
			if (ConnectionExists(nextCell, toCell) ) return true;
		}
	}
	uint32		row = fromCell->Row(), col = fromCell->Col() + 1;
	while (col < mCols) {
		_SeqFilterCell*		nextCell = CellAt(row, col);
		if (nextCell && nextCell->FilterInfo() ) {
			if (ConnectionExists(nextCell, toCell) ) return true;
			if (nextCell->Flags()&LOCK_HRZ_OFF) return false;
		}
		col++;
	}

	return false;
}

void _SeqFilterGrid::DrawOn(BView* view, BRect clip, rgb_color viewColor)
{
	ArpVALIDATE(mPipelineCount <= mRows, return);
	if (mCols < 1) return;
	for (uint32 k = 0; k < mPipelineCount; k++) {
		float		top = mCells[k][0].Top();
		float		bottom = mCells[k][0].Bottom();
		if (top > clip.bottom) return;
		if (bottom >= clip.top) {
			BRect				clippedFrame(clip.left, top, clip.right, bottom);
			float				middle = clippedFrame.top + ((clippedFrame.bottom - clippedFrame.top) / 2);
	
			for (uint32 j = 0; j < mCols; j++)
				mCells[k][j].DrawOn(view, clippedFrame, middle - 3, viewColor, mPipeShade);

			BRect				f = mCells[k][mCols - 1].Frame();
			if (f.right + 1 <= clippedFrame.right)
				seq_draw_hrz_pipe(view, f.right + 1, middle - 3, clippedFrame.right, mPipeShade);
		}
	}
}

_SeqFilterCell* _SeqFilterGrid::CellAt(pipeline_id pid, filter_id fid)
{
	if (mPipelineCount < 1 || mCols < 1) return NULL;
	for (uint32 k = 0; k < mPipelineCount; k++) {
		for (uint32 j = 0; j < mCols; j++) {
			if (mCells[k][j].FilterInfo()  != NULL) {
				if (mCells[k][j].FilterInfo()->mPipelineId == pid) {
					if (mCells[k][j].FilterInfo()->mFilterId == fid)
						return &(mCells[k][j]);
				} else break;
			}
		}
	}
	return NULL;
}

_SeqFilterCell* _SeqFilterGrid::CellAt(uint32 row, uint32 col) const
{
	ArpVALIDATE(row < mRows && col < mCols, return NULL);
	return &(mCells[row][col]);
}

_SeqFilterCell* _SeqFilterGrid::CellAt(BPoint where)
{
	if (mPipelineCount < 1 || mCols < 1) return NULL;
	for (uint32 k = 0; k < mPipelineCount; k++) {
		float		top = mCells[k][0].Top();
		float		bottom = mCells[k][0].Bottom();
		if (where.y < top) return NULL;
		if (where.y <= bottom) {
			for (uint32 j = 0; j < mCols; j++) {
				BRect	f(mCells[k][j].Frame() );
				if (f.Contains(where) ) return &(mCells[k][j]);
			}
		}
	}
	return NULL;
}

_SeqFilterCell* _SeqFilterGrid::CellAt(_SeqFilterLoc* location) const
{
	ArpVALIDATE(location, return NULL);
	if (location->mRow >= int32(mRows) ) return NULL;
	if (location->mCol >= int32(mCols) ) return NULL;
	return &(mCells[location->mRow][location->mCol]);
}

_SeqFilterCell* _SeqFilterGrid::NextValidCell(uint32 row, uint32 col)
{
	for (uint32 k = col; k < mCols; k++) {
		if (mCells[row][k].FilterInfo() ) return &(mCells[row][k]);
	}
	return NULL;
}

BRect _SeqFilterGrid::FrameFor(_SeqFilterLoc* location) const
{
	BRect		f = arp_invalid_rect();
	ArpVALIDATE(location, return f);
	if (location->mRow >= int32(mRows) || location->mCol >= int32(mCols)) return f;
	return mCells[location->mRow][location->mCol].Frame();
}

BRect _SeqFilterGrid::IconFrameFor(_SeqFilterLoc* location) const
{
	BRect		f = arp_invalid_rect();
	ArpVALIDATE(location, return f);
	if (location->mRow >= int32(mRows) || location->mCol >= int32(mCols)) return f;
	return mCells[location->mRow][location->mCol].IconFrame();
}

int32 _SeqFilterGrid::FilterIndexOnOrAfter(uint32 row, uint32 col) const
{
	ArpVALIDATE(row < mRows && col < mCols, return -1);
	for (uint32 c = col; c < mCols; c++) {
		if (mCells[row][c].FilterInfo() ) {
			return mCells[row][c].FilterIndex();
		}
	}
	return -1;
}

#if 0
float _SeqFilterGrid::PixelWidth() const
{
	float		right = 0;
	for (uint32 k = 0; k < mPipelineCount; k++) {
		for (int32 j = int32(mCols) - 1; j >= 0; j--) {
			if (mCells[k][j].FilterInfo() ) {
				BRect	f(mCells[k][j].Frame() );
				if (f.right > right) right = f.right;
				break;
			}
		}
	}
	return right + L_BRACKET + BORDER + FILTER_W + BORDER + R_BRACKET;
}
#endif

void _SeqFilterGrid::GetSize(float* width, float* height) const
{
	float		right = 0, bottom = 0;
	for (uint32 k = 0; k < mPipelineCount; k++) {
		for (int32 j = int32(mCols) - 1; j >= 0; j--) {
			if (mCells[k][j].FilterInfo() ) {
				BRect	f(mCells[k][j].Frame() );
				if (f.right > right) right = f.right;
				if (f.bottom > bottom) bottom = f.bottom;
				break;
			}
		}
	}
	*width = right + L_BRACKET + BORDER + FILTER_W + BORDER + R_BRACKET;
	*height = bottom;
}

void _SeqFilterGrid::Print() const
{
	ArpASSERT(mPipelineCount <= mRows);
	for (uint32 k = 0; k < mPipelineCount; k++) {
		for (uint32 j = 0; j < mCols; j++) {
			if (mCells[k][j].FilterInfo() )
				printf("%ld, %ld %s\t", k, j, mCells[k][j].FilterInfo()->mToolTip.String());
			else
				printf("%ld, %ld\t\t", k, j);
		}
		printf("\n");
	}
}

status_t _SeqFilterGrid::GrowTo(uint32 rows, uint32 cols)
{
	_SeqFilterCell**	newCells = NULL;
	newCells = (_SeqFilterCell**)malloc(sizeof(_SeqFilterCell) * rows);
	if (!newCells) return B_NO_MEMORY;
	for (uint32 k = 0; k < rows; k++) newCells[k] = NULL;
	for (uint32 k = 0; k < rows; k++) {
		newCells[k] = (_SeqFilterCell*)malloc(sizeof(_SeqFilterCell) * cols);
		if (!newCells[k]) return B_NO_MEMORY;
		for (uint32 j = 0; j < cols; j++)
			newCells[k][j].Initialize(k, j, 0, 0);
	}
	uint32				pipelineCount = mPipelineCount;
	if (mCells) {
		for (uint32 k = 0; k < rows; k++) {
			float	top = mCells[k][0].Top();
			float	bottom = mCells[k][0].Bottom();
			for (uint32 j = 0; j < cols; j++) {
				newCells[k][j].Initialize(k, j, top, bottom);
				if (k < mRows && j < mCols)
					newCells[k][j].SwapFilterInfo(mCells[k][j]);
			}
		}
		FreeCells();		
	}
	mCells = newCells;
	mRows = rows;
	mCols = cols;
	mPipelineCount = pipelineCount;
	return B_OK;
}

void _SeqFilterGrid::FreeCells()
{
	if (!mCells) return;
	for (uint32 k = 0; k < mRows; k++) {
		if (mCells[k] != NULL) {
			free(mCells[k]);
			mCells[k] = NULL;
		}
	}
	free(mCells);
	mCells = NULL;
	mRows = mCols = mPipelineCount = 0;
}

// #pragma mark -

/*************************************************************************
 * _SEQ-FILTER-CELL
 *************************************************************************/
_SeqFilterCell::_SeqFilterCell(	uint32 row, uint32 col,
								float top, float bottom)
		: mRow(row), mCol(col), mTop(top), mBottom(bottom),
		  mFlags(0), mFilterInfo(NULL)
{
}

_SeqFilterCell::~_SeqFilterCell()
{
	delete mFilterInfo;
}

void _SeqFilterCell::Initialize(uint32 row, uint32 col, float top, float bottom)
{
	mRow = row;
	mCol = col;
	mTop = top;
	mBottom = bottom;
	mFlags = 0;
	mFilterInfo = NULL;
}

uint32 _SeqFilterCell::Row() const
{
	return mRow;
}

uint32 _SeqFilterCell::Col() const
{
	return mCol;
}

uint32 _SeqFilterCell::Flags() const
{
	return mFlags;
}

void _SeqFilterCell::SetFlag(uint32 flag, bool on)
{
	if (on) mFlags |= flag;
	else mFlags &= ~flag;
}

void _SeqFilterCell::SetFlags(uint32 flag)
{
	mFlags = flag;
}

void _SeqFilterCell::SetMetrics(float top, float bottom)
{
	mTop = top;
	mBottom = bottom;
}

float _SeqFilterCell::Top() const
{
	return mTop;
}

float _SeqFilterCell::Bottom() const
{
	return mBottom;
}

_SeqFilterInfo* _SeqFilterCell::FilterInfo()
{
	return mFilterInfo;
}

void _SeqFilterCell::SetFilterInfo(_SeqFilterInfo* info)
{
	delete mFilterInfo;
	mFilterInfo = info;
}

status_t _SeqFilterCell::CacheFilter(pipeline_id pid, AmFilterHolderI* holder, int32 filterIndex)
{
	delete mFilterInfo;
	mFilterInfo = NULL;
	mFilterInfo = new _SeqFilterInfo(pid, holder, filterIndex, Row() );
	if (!mFilterInfo) return B_NO_MEMORY;
	return B_OK;
}

void _SeqFilterCell::SwapFilterInfo(_SeqFilterCell& cell)
{
	_SeqFilterInfo*	t = mFilterInfo;
	mFilterInfo = cell.mFilterInfo;
	cell.mFilterInfo = t;
}

pipeline_id _SeqFilterCell::PipelineId() const
{
	if (!mFilterInfo) return 0;
	return mFilterInfo->mPipelineId;
}

filter_id _SeqFilterCell::FilterId() const
{
	if (!mFilterInfo) return 0;
	return mFilterInfo->mFilterId;
}

int32 _SeqFilterCell::FilterIndex() const
{
	if (!mFilterInfo) return -1;
	return mFilterInfo->mFilterIndex;
}

bool _SeqFilterCell::IsBypassed() const
{
	if (!mFilterInfo) return false;
	return mFilterInfo->mBypassed;
}

bool _SeqFilterCell::HasToolTip() const
{
	if (!mFilterInfo) return false;
	return mFilterInfo->mToolTip.Length() > 0;
}

const char* _SeqFilterCell::ToolTip() const
{
	if (!mFilterInfo) return NULL;
	return mFilterInfo->mToolTip.String();
}

uint32 _SeqFilterCell::FilterFlags() const
{
	if (!mFilterInfo) return 0;
	return mFilterInfo->mFlags;
}

bool _SeqFilterCell::HitPropertyMenu(BPoint where) const
{
	if (!gPropertyMenuBm) return false;
	BRect			f(gPropertyMenuBm->Bounds() );
	f.OffsetBy(PropertyMenuOrigin() );
	return f.Contains(where);
}

BPoint _SeqFilterCell::PropertyMenuOrigin() const
{
	return IconFrame().LeftTop();
}

bool _SeqFilterCell::HitConnection(BPoint where) const
{
	if (!gFilterConnectionBm) return false;
	if (!CanMakeConnection() ) return false;
	BRect			f(gFilterConnectionBm->Bounds() );
	f.OffsetBy(ConnectionOrigin() );
	return f.Contains(where);
}

BPoint _SeqFilterCell::ConnectionOrigin() const
{
	BRect		frame(IconFrame() );
	if (!gFilterConnectionBm) return frame.LeftTop();
	float		y = (frame.top + (frame.Height() / 2)) - (gFilterConnectionBm->Bounds().Height() / 2);
	return BPoint(frame.left + L_BRACKET + BORDER + FILTER_W + BORDER, y);
}

bool _SeqFilterCell::CanMakeConnection() const
{
	if (!mFilterInfo) return false;
	return mFilterInfo->CanMakeConnection();
}

void _SeqFilterCell::DrawOn(BView* view, BRect clippedFrame, float pipelineTop, rgb_color viewColor,
							float pipeShade)
{
	BRect		frame(Frame() );
	if (!frame.Intersects(clippedFrame)) return;

	if (!(mFlags&LOCK_HRZ_OFF) )
		seq_draw_hrz_pipe(view, frame.left, pipelineTop, frame.right, pipeShade);
	if (!mFilterInfo) {
		draw_cleanup_pipe(view, frame.left, pipelineTop, mFlags, pipeShade);
		if (mFlags&CAP_LEFT) {
			view->SetHighColor( seq_darken(Prefs().Color(AM_PIPELINE_C), PIPELINE_SHADE_3 * pipeShade) );
			view->StrokeLine(BPoint(frame.left, pipelineTop + 1), BPoint(frame.left, pipelineTop + PIPELINE_H - 2));
		}
	}
	
	BRect		iconF(IconFrame() );
	BRect		f(frame.left + L_BRACKET, iconF.top, frame.left + L_BRACKET + BORDER + FILTER_W, iconF.top + BORDER + FILTER_H);
	if (mFilterInfo) {
		mFilterInfo->DrawOn(view, f, viewColor);
		if (IsBypassed() ) seq_draw_hrz_pipe(view, f.left, pipelineTop, f.right, pipeShade);
	}
	/* Draw the pipeline connections, if any.
	 */
	f.left = f.right + 1;
	f.right = frame.right;
	f.top = frame.top;
	f.bottom = frame.bottom;
	draw_vrt_pipe(view, f, pipelineTop, mFlags, pipeShade);
}


AmFilterHolder* _SeqFilterCell::FilterHolder(	const AmPipelineMatrixI* matrix,
												AmPipelineType type) const
{
	ArpASSERT(matrix);
	if (!mFilterInfo) return NULL;
	return dynamic_cast<AmFilterHolder*>( matrix->Filter(mFilterInfo->mPipelineId, type, mFilterInfo->mFilterId) );
}


BRect _SeqFilterCell::Frame() const
{
	return BRect(mCol * COL_W, mTop, (mCol * COL_W) + COL_W - 1, mBottom);
}

BRect _SeqFilterCell::IconFrame() const
{
	float		top = mTop + (((mBottom - mTop) / 2) - (FILTER_H / 2)) - 1;
	return BRect(mCol * COL_W, top, (mCol * COL_W) + COL_W - 1, top + FILTER_H + 1);
}

// #pragma mark -

/*************************************************************************
 * _SEQ-FILTER-INFO
 *************************************************************************/
_SeqFilterInfo::_SeqFilterInfo(	const AmFilterHolderI* holder,
								int32 filterIndex, uint32 row)
		: mPipelineId(0), mFilterId(0), mFilterIndex(filterIndex),
		  mBypassed(false), mIcon(NULL), mMaxConnections(1), mFlags(0)
{
	if (holder && holder->Filter() ) {
		mPipelineId = holder->PipelineId();
		mFilterId = holder->Filter()->Id();
		mBypassed = holder->IsBypassed();
		holder->Filter()->GetToolTipText(&mToolTip);
		int32		bgIndex = 0;
		int32		bgCount = Prefs().Int32(AM_FILTER_BG_COUNT_I32);
		if (bgCount > 0) bgIndex = row % bgCount;
		mIcon = ArpMakeFilterBitmap(holder->Filter(), BPoint(FILTER_W, FILTER_H), bgIndex);
		if (holder->Filter()->AddOn() ) mMaxConnections = holder->Filter()->AddOn()->MaxConnections();
		mFlags = holder->Filter()->Flags();
	}
}

_SeqFilterInfo::_SeqFilterInfo(	pipeline_id pipelineId,
								const AmFilterHolderI* holder,
								int32 filterIndex, uint32 row)
		: mPipelineId(pipelineId), mFilterId(0), mFilterIndex(filterIndex),
		  mBypassed(false), mIcon(NULL), mMaxConnections(1), mFlags(0)
{
	if (holder && holder->Filter() ) {
		mFilterId = holder->Filter()->Id();
		mBypassed = holder->IsBypassed();
		holder->Filter()->GetToolTipText(&mToolTip);
		int32		bgIndex = 0;
		int32		bgCount = Prefs().Int32(AM_FILTER_BG_COUNT_I32);
		if (bgCount > 0) bgIndex = row % bgCount;
		mIcon = ArpMakeFilterBitmap(holder->Filter(), BPoint(FILTER_W, FILTER_H), bgIndex);
		if (holder->Filter()->AddOn() ) mMaxConnections = holder->Filter()->AddOn()->MaxConnections();
		MakeConnections(holder);
		mFlags = holder->Filter()->Flags();
	}
}

_SeqFilterInfo::~_SeqFilterInfo()
{
	delete mIcon;
}

void _SeqFilterInfo::DrawOn(BView* view, BRect frame, rgb_color viewColor)
{
	/* Draw the icon.
	 */
	if (mIcon) view->DrawBitmapAsync(mIcon, BPoint(frame.left + 1, frame.top + 1));
	view->Sync();
	/* Draw a border around the icon.
	 */
	view->SetHighColor( tint_color(viewColor, B_LIGHTEN_2_TINT) );
	view->StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.top) );
	view->StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.left, frame.bottom) );
	view->SetHighColor(0, 0, 0);
	view->StrokeLine(BPoint(frame.left + 1, frame.bottom), BPoint(frame.right, frame.bottom) );
	view->StrokeLine(BPoint(frame.right, frame.bottom), BPoint(frame.right, frame.top + 1) );
}

bool _SeqFilterInfo::CanMakeConnection() const
{
	if (mMaxConnections < 0) return true;
	if (mMaxConnections == 0) return false;
	if (mMaxConnections == 1) return true;
	/* Plus one because there's always an implied connection -- the
	 * next in the pipeline.
	 */
	return int32(mConnections.size() + 1) < mMaxConnections;
}

void _SeqFilterInfo::MakeConnections(const AmFilterHolderI* holder)
{
	mConnections.resize(0);
	uint32			count = holder->CountConnections();
	for (uint32 k = 0; k < count; k++) {
		AmFilterHolderI*	h = holder->ConnectionAt(k);
		if (h && h->Filter() && holder->PipelineId() != h->PipelineId() ) {
			mConnections.push_back(_SeqFilterLoc(	h->PipelineId(),
													h->Filter()->Id(),
													h->Filter()->Label() ) );
		}
	}
}

// #pragma mark -

/*************************************************************************
 * _SEQ-FILTER-LOC
 *************************************************************************/
_SeqFilterLoc::_SeqFilterLoc()
		: mPid(0), mFid(0), mRow(-1), mCol(-1),
		  mNoProperties(false), mNoConnections(false)
{
}

_SeqFilterLoc::_SeqFilterLoc(const _SeqFilterLoc& o)
		: mPid(o.mPid), mFid(o.mFid), mRow(o.mRow), mCol(o.mCol),
		  mNoProperties(o.mNoProperties), mNoConnections(o.mNoConnections),
		  mLabel(o.mLabel)
{
}

_SeqFilterLoc::_SeqFilterLoc(pipeline_id pid, filter_id fid)
		: mPid(pid), mFid(fid), mRow(0), mCol(0),
		  mNoProperties(false), mNoConnections(false)
{
}

_SeqFilterLoc::_SeqFilterLoc(	pipeline_id pid, filter_id fid,
								const BString& label)
		: mPid(pid), mFid(fid), mRow(0), mCol(0),
		  mNoProperties(false), mNoConnections(false),
		  mLabel(label)
{
}

_SeqFilterLoc::_SeqFilterLoc(	pipeline_id pid, filter_id fid,
								uint32 row, uint32 col,
								const BString& label)
		: mPid(pid), mFid(fid), mRow(row), mCol(col),
		  mNoProperties(false), mNoConnections(false),
		  mLabel(label)
{
}

_SeqFilterLoc::_SeqFilterLoc(const _SeqFilterCell& cell)
		: mPid(cell.PipelineId() ), mFid(cell.FilterId() ),
		  mRow(cell.Row() ), mCol(cell.Col() ),
		  mNoProperties(false), mNoConnections(false)
{
}

_SeqFilterLoc& _SeqFilterLoc::operator=(const _SeqFilterLoc& o)
{
	mPid = o.mPid;
	mFid = o.mFid;
	mRow = o.mRow;
	mCol = o.mCol;
	mNoProperties = o.mNoProperties;
	mNoConnections = o.mNoConnections;
	mLabel = o.mLabel;
	return *this;
}

bool _SeqFilterLoc::Equals(const _SeqFilterLoc& o) const
{
	return	mPid == o.mPid && mFid == o.mFid
			&& mRow == o.mRow && mCol == o.mCol;
}

// #pragma mark -

/*************************************************************************
 * THE LAYOUT ALGORITHM
 * I start by caching all the filters in the matrix into a simple grid.
 * The filter's column is its minimum possible final column.  Then I run
 * through each row in the first column, find the connections, and increment
 * each connection's column number to 1 plus + mine.  Then it's just a
 * matter of copying over my info into a real grid.
 *************************************************************************/
class _LayoutCell
{
public:
	_LayoutCell() : mRow(0), mCol(0), mMinCol(0), mHolder(NULL) { }
	
	uint32					mRow, mCol;
	uint32					mMinCol;
	AmFilterHolderI*		mHolder;
	vector<_LayoutCell*>	mConnections;
};

status_t _SeqFilterGrid::NewLayout(const AmPipelineMatrixI* matrix)
{
	/* Construct a grid of my filters.
	 */
	uint32			cols, rows;
	_LayoutCell**	cells = GetLayoutCells(matrix, &cols, &rows);
	if (!cells) return B_NO_MEMORY;
	uint32			newCols = 0;
	/* This is the basic algorithm for the layout -- see that method for
	 * an explanation.
	 */
	while (ShiftCells(cells, rows, cols) == true) ;
	/* Find out the width of my new grid.
	 */
	for (uint32 r = 0; r < rows; r++) {
		for (uint32 c = 0; c < cols; c++) {
			if (newCols < cells[r][c].mMinCol) newCols = cells[r][c].mMinCol + 1;
		}
	}
	/* Copy my layout into a new cells structure.
	 */
	FreeCells();
	status_t	err = GrowTo(rows + mGrowBy, newCols + mGrowBy);
	if (err != B_OK) {
		FreeLayoutCells(cells, rows, cols);
		return err;
	}
	/* Fill in the correct size info for the rows.
	 */
	mPipelineCount = matrix->CountPipelines();
	float			top = 0, bottom = 0;
	pipeline_id		pid;
	for (uint32 r = 0; (pid = matrix->PipelineId(r)) != 0; r++) {
		float		height;
		if (matrix->PipelineHeight(r, &height) == B_OK) {
			bottom = top + height;
			if (r < mRows) {
				for (uint32 c = 0; c < mCols; c++)
					mCells[r][c].SetMetrics(top, bottom);
			}
			top = bottom + 1;
		}
	}
	/* Copy all my layout info into the grid.
	 */
	for (uint32 c = 0; c < cols; c++) {
		for (uint32 r = 0; r < rows; r++) {
			if (cells[r][c].mHolder)
				CacheLayoutCell(cells[r][c], r, int32(c), cells, rows, cols);
		}
	}
	/* Connect all the pipes.
	 */
	for (uint32 k = 0; k < mRows; k++) {
		for (uint32 j = 0; j < mCols; j++) {
			LayoutPipes(k, j);
		}
	}
	
	FreeLayoutCells(cells, rows, cols);
	return B_OK;
}

static bool filter_between(	_LayoutCell& fromCell, _LayoutCell& toCell,
							_LayoutCell** cells, uint32 rows, uint32 cols)
{
	ArpVALIDATE(toCell.mMinCol > 0, return false);
	if (toCell.mCol < 1) return false;
	if (cells[toCell.mRow][toCell.mCol - 1].mMinCol > fromCell.mMinCol) return true;
	return false;
}

bool _SeqFilterGrid::ShiftCells(_LayoutCell** cells, uint32 rows, uint32 cols) const
{
	bool		more = false;
	/* 1.  Run through every column in each row, making sure each cell's column is
	 * greater than the column preceding it, and giving all of each cell's connections
	 * a column number greater than or equal to the cell's column.
	 */
	for (uint32 c = 0; c < cols; c++) {
		for (uint32 r = 0; r < rows; r++) {
			if (cells[r][c].mHolder) {
				_LayoutCell&		cur = cells[r][c];
				if (cur.mMinCol < c) {
					more = true;
					cur.mMinCol = c;
				}
				if (c > 0 && cur.mMinCol <= cells[r][c-1].mMinCol) {
					more = true;
					cur.mMinCol = cells[r][c-1].mMinCol + 1;
				}
				for (uint32 k = 0; k < cells[r][c].mConnections.size(); k++) {
					_LayoutCell*	branch = cur.mConnections[k];
					if (branch) {
						if (cur.mMinCol >= branch->mMinCol) {
							branch->mMinCol = cur.mMinCol + 1;
							more = true;
						}
					}
				}
			}
		}
	}
	/* 2.  While there exists someone who has filters in the columns between them and
	 * a connection, I need to shift that filter over and clean up all the column
	 * positions (i.e. make sure every filter is in a later column then their
	 * previous filters).  If the matrix is invalid -- i.e. there are cycles -- this
	 * will send the layout into an infinite loop.  I need to think of a way to deal
	 * with that.
	 */
	for (uint32 c = 0; c < cols; c++) {
		for (uint32 r = 0; r < rows; r++) {
			_LayoutCell&		cur = cells[r][c];
			for (uint32 k = 0; k < cells[r][c].mConnections.size(); k++) {
				_LayoutCell*	branch = cur.mConnections[k];
				if (branch) {
					if (filter_between(cur, *branch, cells, rows, cols)) {
						ArpASSERT(branch->mMinCol > cur.mMinCol + 1);
						cur.mMinCol = branch->mMinCol - 1;
						more = true;
					}
				}
			}
		}
	}
	return more;
}

void _SeqFilterGrid::LayoutPipes(uint32 row, uint32 col)
{
	ArpASSERT(row < mRows && col < mCols);
	_SeqFilterInfo*		info = mCells[row][col].FilterInfo();
	if (!info) return;
	if (info->mConnections.size() < 1) return;

	/* If I have a max connection of 1, but I have a connection,
	 * that means this pipe is being diverted completely to another
	 * pipeline.  In this case, I want to turn off the drawing of
	 * my horizontal pipeline.
	 */
	if (info->mMaxConnections == 1) {
		mCells[row][col].SetFlag(LOCK_HRZ_OFF, true);
		if (col + 1 < mCols) mCells[row][col + 1].SetFlag(CAP_LEFT, true);
	}

	/* The in pipes are one before the actual filter, just because
	 * that's the cell that does the drawing.
	 */
	for (uint32 k = 0; k < info->mConnections.size(); k++) {
		if (info->mConnections[k].mRow < int32(row)) {
			mCells[row][col].SetFlag(OUT_UP, true);
			int32		i = col;
			ArpVALIDATE(i >= 0, i = 0);
			uint32		r = info->mConnections[k].mRow;
			for (int32 j = r + 1; j < int32(row); j++) {
				mCells[j][i].SetFlag(THROUGH_UP, true);
				mCells[j][i].SetFlag(THROUGH_DOWN, true);
			}
			mCells[r][i].SetFlag(IN_DOWN, true);
			if (i + 1 < int32(mCols)) mCells[r][i + 1].SetFlag(IN_DOWN_CLEANUP, true);
		} else {
			mCells[row][col].SetFlag(OUT_DOWN, true);
			int32		i = col;
			ArpVALIDATE(i >= 0, i = 0);
			int32		r = info->mConnections[k].mRow;
			for (int32 j = row + 1; j < r; j++) {
				mCells[j][i].SetFlag(THROUGH_UP, true);
				mCells[j][i].SetFlag(THROUGH_DOWN, true);
			}
			mCells[r][i].SetFlag(IN_UP, true);
			if (i + 1 < int32(mCols)) mCells[r][i + 1].SetFlag(IN_UP_CLEANUP, true);
		}
	}
}

void _SeqFilterGrid::CacheLayoutCell(	_LayoutCell& cell, uint32 row, int32 filterIndex,
										_LayoutCell** cells, uint32 rows, uint32 cols)
{
	ArpASSERT(cell.mHolder);
	ArpVALIDATE(row < mRows && cell.mMinCol < mCols, return);
	_SeqFilterInfo*		info = new _SeqFilterInfo(cell.mHolder, filterIndex, cell.mRow);
	if (!info) return;
	for (uint32 k = 0; k < cell.mConnections.size(); k++) {
		_LayoutCell*		con = cell.mConnections[k];
		if (con && con->mHolder) {
			info->mConnections.push_back(_SeqFilterLoc(	con->mHolder->PipelineId(),
														con->mHolder->Filter()->Id(),
														con->mRow, con->mMinCol,
														con->mHolder->Filter()->Label()));
		}
	}
	mCells[row][cell.mMinCol].SetFilterInfo(info);
}

_LayoutCell** _SeqFilterGrid::GetLayoutCells(	const AmPipelineMatrixI* matrix,
												uint32* w, uint32* h) const
{
	ArpASSERT(matrix && w && h);
	uint32						rows = 0, cols = 0;
	rows = matrix->CountPipelines();
	if (rows < 1) return NULL;
	for (uint32 k = 0; k < rows; k++) {
		pipeline_id				pid = matrix->PipelineId(k);
		if (pid) {
			AmFilterHolderI*	holder = matrix->Filter(pid, mPipelineType);
			uint32				fi = 0;
			while (holder) {
				if (holder->Filter() && accept_addon_type(mPipelineType, holder->Type()) )
					fi++;
				holder = holder->NextInLine();
			}
			if (fi > cols) cols = fi;
		}
	}
	if (cols < 1) cols = mGrowBy;
	_LayoutCell**				newCells = NULL;
	newCells = (_LayoutCell**)malloc(sizeof(_LayoutCell) * rows);
	if (!newCells) return NULL;
	for (uint32 k = 0; k < rows; k++) newCells[k] = NULL;
	for (uint32 k = 0; k < rows; k++) {
		newCells[k] = new _LayoutCell[cols];
		if (!newCells[k]) {
			FreeLayoutCells(newCells, rows, cols);
			return NULL;
		}
	}
	/* Assign the filters to their appropriate cells.
	 */
	for (uint32 k = 0; k < rows; k++) {
		pipeline_id				pid = matrix->PipelineId(k);
		if (pid) {
			AmFilterHolderI*	holder = matrix->Filter(pid, mPipelineType);
			uint32				fi = 0;
			while (holder) {
				if (holder->Filter() && accept_addon_type(mPipelineType, holder->Type()) ) {
					newCells[k][fi].mHolder = holder;
					fi++;
				}
				holder = holder->NextInLine();
			}
		}
	}
	/* Setup the connection locations for each cell.
	 */
	for (uint32 k = 0; k < rows; k++) {
		for (uint32 j = 0; j < cols; j++) {
			newCells[k][j].mRow = k;
			newCells[k][j].mCol = j;
			if (newCells[k][j].mHolder != NULL) {
				MakeLayoutConnections(matrix, newCells, rows, cols, &(newCells[k][j]));
			}
		}
	}

	*w = cols;
	*h = rows;
	return newCells;
}

void _SeqFilterGrid::MakeLayoutConnections(	const AmPipelineMatrixI* matrix,
											_LayoutCell** cells, uint32 rows, uint32 cols,
											_LayoutCell* cell) const
{
	ArpASSERT(cell && cell->mHolder != NULL);
	uint32			count = cell->mHolder->CountConnections();
	for (uint32 k = 0; k < count; k++) {
		AmFilterHolderI*	h = cell->mHolder->ConnectionAt(k);
		if (h && h->Filter() && cell->mHolder->PipelineId() != h->PipelineId() ) {
			PushBackConnection(cell, h, cells, rows, cols);
		}
	}
}

void _SeqFilterGrid::PushBackConnection(_LayoutCell* cell, AmFilterHolderI* h,
										_LayoutCell** cells, uint32 rows, uint32 cols) const
{
	for (uint32 r = 0; r < rows; r++) {
		if (cells[r][0].mHolder && cells[r][0].mHolder->PipelineId() == h->PipelineId() ) {
			for (uint32 c = 0; c < cols; c++) {
				if (cells[r][c].mHolder && cells[r][c].mHolder->Filter()->Id() == h->Filter()->Id() ) {
					cell->mConnections.push_back(&cells[r][c]);
					return;
				}
			}
		}
	}
printf("FAILED TO PUSH BACK CONNECTION FOR %s (fid: %p)\n", h->Filter()->Name().String(), h->Filter()->Id());
	ArpASSERT(false);
}

void _SeqFilterGrid::FreeLayoutCells(_LayoutCell** cells, uint32 rows, uint32 cols) const
{
	if (!cells) return;
	for (uint32 k = 0; k < rows; k++) {
		if (cells[k] != NULL) {
			delete[] cells[k];
			cells[k] = NULL;
		}
	}
	free(cells);
}

void _SeqFilterGrid::PrintLayoutCells(_LayoutCell** cells, uint32 rows, uint32 cols) const
{
	if (!cells) {
		printf("Layout cells: NULL\n");
		return;
	}

	for (uint32 r = 0; r < rows; r++) {
		printf("ROW %ld: ", r);
		if (!cells[r]) printf("NULL");
		else {
			for (uint32 c = 0; c < cols; c++) {
				if (!cells[r][c].mHolder) printf("(EMPTY)\t\t\t");
				else {
					printf("%s - \n", cells[r][c].mHolder->Filter()->Name().String() );
					for (uint32 k = 0; k < cells[r][c].mConnections.size(); k++) {
						if (!cells[r][c].mConnections[k]->mHolder) printf("<error> ");
						else printf("%s ", cells[r][c].mConnections[k]->mHolder->Filter()->Name().String() );
					}
				}
			}
		}
		printf("\n");
	}
}
