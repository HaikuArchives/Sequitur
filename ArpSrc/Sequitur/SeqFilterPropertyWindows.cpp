/* SeqFilterPropertyWindows.cpp
 */
#include <stdlib.h>
#include "ArpCollections/ArpPtrVector.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpBaseLayout.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmMultiFilter.h"
#include "Sequitur/SeqFilterPropertyWindows.h"
#include "Sequitur/SeqSplitterView.h"
#include "Sequitur/SequiturDefs.h"

ArpMOD();

static const char*		TABLE_STR				= "table";
static const char*		PAGE_STR				= "Page";

static const uint32		NAME_COL_INDEX			= 0;

static BRect figure_frame(BWindow* parent)
{
	if (parent) return parent->Frame();
	BScreen		s;
	BRect		f = s.Frame();
	return BRect(f.left * 0.35, f.top * 0.35, f.right * 0.65, f.bottom * 0.65);
}

/********************************************************
 * _FILTER-PAGE-LIST-VIEW
 ********************************************************/
class _FilterPageListView : public BColumnListView
{
public:
	_FilterPageListView(BRect rect, const char *name,
						SeqAbstractFilterPropertyWindow* window);
	
	virtual void SelectionChanged();
	
private:
	typedef BColumnListView	inherited;
	SeqAbstractFilterPropertyWindow* mWindow;
};

/********************************************************
 * _FILTER-PAGE-ROW
 ********************************************************/
class _FilterPageRow : public BRow
{
public:
	_FilterPageRow(const char* name, BView* view, float width);
	virtual ~_FilterPageRow();
	
	virtual bool		HasLatch() const;
	BView*				View() const;
	float				Width() const;
		
protected:
	BView*				mView;
	float				mWidth;
};

/******************************************************************
 * SEQ-ABSTRACT-FILTER-PROPERTY-WINDOW
 ******************************************************************/
SeqAbstractFilterPropertyWindow::SeqAbstractFilterPropertyWindow(
											BWindow* parent,
											window_look look,
											window_feel feel,
											uint32 flags)
	: BWindow(figure_frame(parent), "Properties", look, feel, flags, B_CURRENT_WORKSPACE),
	  mBg(NULL), mListView(NULL), mSplitter(NULL),
	  mPageView(NULL), mBlankPageView(NULL)
{
	if (parent) AddToSubset(parent);

	AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO));
	AddShortcut('Z', B_SHIFT_KEY|B_COMMAND_KEY, new BMessage('REDO'));

	mBlankPageView = new BView(BRect(0, 0, 0, 0), "blank", B_FOLLOW_ALL, 0);
	if (mBlankPageView) mBlankPageView->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
}

SeqAbstractFilterPropertyWindow::~SeqAbstractFilterPropertyWindow()
{
	if (mPageView) mPageView->RemoveSelf();
	if (mBlankPageView) delete mBlankPageView;
}

void SeqAbstractFilterPropertyWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SEQ_ACTIVATE:
			Activate();
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqAbstractFilterPropertyWindow::QuitRequested()
{
	return true;
}

void SeqAbstractFilterPropertyWindow::SetPage(BView* view)
{
	if (!mBg || !mSplitter) return;
	if (!view) view = mBlankPageView;
	if (!view) return;
	BRect		targetF(CurrentPageFrame() );
	BRect		pageF = view->Frame();
	view->SetResizingMode(B_FOLLOW_ALL);
	if (pageF.LeftTop() != targetF.LeftTop() ) view->MoveTo(targetF.LeftTop() );
	if (mPageView) mBg->RemoveChild(mPageView);
	mBg->AddChild(view);
	if (pageF.Width() != targetF.Width() || pageF.Height() != targetF.Height() )
		view->ResizeTo(targetF.Width(), targetF.Height() );
	mPageView = view;
}

BRect SeqAbstractFilterPropertyWindow::CurrentPageFrame() const
{
	if (!mSplitter) return arp_invalid_rect();
	BRect		f = Bounds();
	BRect		splitterF = mSplitter->Frame();
	return BRect(splitterF.right + 1, splitterF.top, f.right - 5, splitterF.bottom);
}

status_t SeqAbstractFilterPropertyWindow::SetFirstPage()
{
	if (!mListView) return B_ERROR;
	BRow*		r = mListView->RowAt(0);
	if (!r) return B_ERROR;
	mListView->AddToSelection(r);
	mListView->SelectionChanged();
	/* By default, the width of the mListView is just wide enough
	 * to display the longest page name.  (But not greater than half
	 * the total width of the view)
	 */
	float		w = 0;
	for (int32 k = 0; (r = mListView->RowAt(k)) != NULL; k++) {
		_FilterPageRow*	tp = dynamic_cast<_FilterPageRow*>(r);
		if (tp && tp->Width() > w) w = tp->Width();
	}
	BRect		b = Bounds();
	float		half = b.Width() / 2;
	w += 50;		// Account for the left area of the list view
	if (w < 20) w = 20;
	else if (w > half) w = half;
	if (mSplitter && mPageView) {
		mListView->ResizeTo(w, mListView->Frame().Height() );
		mSplitter->MoveTo(mListView->Frame().right + 1, mSplitter->Frame().top);
		BRect		pf = mPageView->Frame();
		BRect		sf = mSplitter->Frame();
		mPageView->MoveTo(sf.right + 1, pf.top);
		mPageView->ResizeTo(b.Width() - sf.right, pf.Height() );
	}
	return B_OK;
}

void SeqAbstractFilterPropertyWindow::Init()
{
	BRect				b(Bounds() );
	BRect				r(b);
	r.bottom = b.bottom - B_H_SCROLL_BAR_HEIGHT;
	BPoint				prefSize;
	AddViews(r, prefSize);
	/* This accounts for the document window tab.  That tab does
	 * not play nicely with a background that goes completely behind
	 * it -- it leaves trails -- so we have to chunk the background
	 * into two parts.
	 */
	r.top = r.bottom + 1;
	r.bottom = b.bottom;
	r.right = b.right - B_V_SCROLL_BAR_WIDTH;
	BView*		v = new BView(r, "filler", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, 0);
	if (v) {
		v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		AddChild(v);
	}

	SetFirstPage();

	/* Set the window size.
	 */
	float			minH = B_H_SCROLL_BAR_HEIGHT * 4;
	float			bufferH = B_H_SCROLL_BAR_HEIGHT * 3;
	if (mSplitter) prefSize.x += mSplitter->Frame().right;
	prefSize.y += bufferH;
	if (prefSize.x < 30) prefSize.x = 30;
	if (prefSize.y < minH) prefSize.y = minH;
	ResizeTo(prefSize.x, prefSize.y);
	float			minw=0,minh=0,maxw=0,maxh=0;
	GetSizeLimits(&minw,&maxw,&minh,&maxh);
	SetSizeLimits(prefSize.x, maxw, prefSize.y, maxh);
	SetZoomLimits(prefSize.x, prefSize.y);
}

void SeqAbstractFilterPropertyWindow::AddFilterPage(AmFilterI* filter,
													BColumnListView* list, BRow* parent,
													BPoint& pt, BWindow* win)
{
	ArpASSERT(filter);
	if (filter->Flags()&AmFilterI::HIDE_PROPERTIES_FLAG) return;

	ArpPtrVector<BView*>	getViews;
	if (filter->Configure(getViews) == B_OK && getViews.size() > 0) {
		if (getViews.size() == 1) {
			AddRow(filter->Label(), getViews[0], list, parent, pt, win);
		} else {
			_FilterPageRow*		parentRow = AddRow(filter->Label(), NULL, list, parent, pt, win);
			if (parentRow) {
				for (uint32 k = 0; k < getViews.size(); k++) {
					if (getViews[k]) {
						BString		n = getViews[k]->Name();
						AddRow(n, getViews[k], list, parentRow, pt, win);
					}
				}
			}
		}
	}
}

void SeqAbstractFilterPropertyWindow::AddMultiFilterPage(	AmMultiFilter* filter,
															BColumnListView* list, BRow* parent,
															BPoint& pt, BWindow* win)
{
	ArpASSERT(filter);
	if (filter->Flags()&AmFilterI::HIDE_PROPERTIES_FLAG) return;
	_FilterPageRow*		newParent = AddRow(filter->Label(), NULL, list, parent, pt, win);
	if (newParent) {
		uint32			count = filter->CountPipelines();
		for (uint32 k = 0; k < count; k++) {
			AmFilterHolderI*	holder = filter->Filter(filter->PipelineId(k), NULLINPUTOUTPUT_PIPELINE);
			while (holder) {
				if (holder->Filter() ) {
					AmMultiFilter*		mf = dynamic_cast<AmMultiFilter*>(holder->Filter() );
					if (mf) AddMultiFilterPage(mf, list, newParent, pt, win);
					else AddFilterPage(holder->Filter(), list, newParent, pt, win);
				}
				holder = holder->NextInLine();
			}
		}
	}
}

void SeqAbstractFilterPropertyWindow::AddViews(BRect frame, BPoint& pt)
{
	float				x = Prefs().Size(SPACE_X), y = Prefs().Size(SPACE_Y);
	float				tableW = (frame.Width() - x - x) / 2;
	BRect				tableF(x, y, x + tableW, frame.Height() - y);
	BRect				splitterF(tableF.right + 1, tableF.top, tableF.right + 5, tableF.bottom);
	pt.x = 0;
	pt.y = 0;
	mBg = new BView(frame, "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!mBg) return;
	mBg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	AddChild(mBg);

	mListView = new _FilterPageListView(tableF, TABLE_STR, this);
	if (!mListView) return;
	mBg->AddChild(mListView);
	mListView->AddColumn(new BStringColumn(PAGE_STR, 110, 20, 350, B_TRUNCATE_MIDDLE), NAME_COL_INDEX);

	mSplitter = new SeqSplitterView(splitterF, "v_splitter", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, B_VERTICAL);
	if (mSplitter) {
		mSplitter->SetDrawingFlags(SeqSplitterView::NO_DRAWING_FLAG);
		mSplitter->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		mBg->AddChild(mSplitter);
	}

	AddPages(mListView, pt);
}

_FilterPageRow* SeqAbstractFilterPropertyWindow::AddRow(const BString& label, BView* view, BColumnListView* list,
								BRow* parent, BPoint& pt, BWindow* win)
{
	float				colWidth = 0;
	if (label.Length() > 0) colWidth = list->StringWidth(label.String() );
	_FilterPageRow*		newRow = new _FilterPageRow(label.String(), view, colWidth);
 	if (!newRow) return NULL;
 	list->AddRow(newRow, parent);
	if (view) {
		float			w = 0, h = 0;
		ArpBaseLayout*	bl = dynamic_cast<ArpBaseLayout*>(view);
		if (bl && win) {
			win->AddChild(view);
			w = bl->LayoutDimens().X().TotalPref();
			h = bl->LayoutDimens().Y().TotalPref();
			win->RemoveChild(view);
		} else {
			view->GetPreferredSize(&w, &h);
		}
		if (w > pt.x) pt.x = w;
		if (h > pt.y) pt.y = h;
	}
	return newRow;
}

// #pragma mark -

/******************************************************************
 * SEQ-PIPELINE-PROPERTY-WINDOW
 ******************************************************************/
SeqPipelinePropertyWindow::SeqPipelinePropertyWindow(	const AmPipelineMatrixRef& matrixRef,
														AmPipelineType pipelineType,
														const char* pipelineName,
														BWindow* parent)
	: inherited(parent),
	  mMatrixRef(matrixRef), mPipelineType(pipelineType)
{
	Init();

	BString		title(pipelineName);
	title << " Properties";
	SetTitle(title.String() );
}

void SeqPipelinePropertyWindow::AddPages(BColumnListView* list, BPoint& pt)
{
	ArpASSERT(list);
	BWindow*		win = new BWindow(BRect(0, 0, 2, 2), "NO", B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0);
	// WRITE PIPELINE BLOCK
	AmPipelineMatrixI*		matrix = mMatrixRef.WriteLock();
	if (matrix) {
		pipeline_id			pid;
		for (uint32 k = 0; (pid = matrix->PipelineId(k)) != 0; k++) {
			AmFilterHolderI*	holder = matrix->Filter(pid, mPipelineType);
			while (holder) {
				if (holder->Filter() ) {
					AmMultiFilter*		mf = dynamic_cast<AmMultiFilter*>(holder->Filter() );
					if (mf) AddMultiFilterPage(mf, list, NULL, pt, win);
					else AddFilterPage(holder->Filter(), list, NULL, pt, win);
				}
				holder = holder->NextInLine();
			}
		}
	}
	mMatrixRef.WriteUnlock(matrix);
	// END WRITE PIPELINE BLOCK

	if (win) win->Quit();
}

// #pragma mark -

/******************************************************************
 * SEQ-FILTER-PROPERTY-WINDOW
 ******************************************************************/
SeqFilterPropertyWindow::SeqFilterPropertyWindow(	const AmPipelineMatrixRef& matrixRef,
													AmPipelineType pipelineType,
													AmFilterHolderI* filter,
													BWindow* parent)
	: inherited(parent),
	  mMatrixRef(matrixRef), mFilter(filter)
{
	mFilter->IncRefs();
	if (mFilter->Filter() ) mFilter->Filter()->ConfigWindowOpened();

	Init();

	BString		title;
	if (mFilter->Filter() ) title = mFilter->Filter()->Label();
	if (title.Length() > 0) title << " ";
	title << "Filter Properties";

#if 0
				switch (mPipelineType) {
					case INPUT_PIPELINE:
						title << ": " << "Input";
						break;
					case THROUGH_PIPELINE:
						title << ": " << "Through";
						break;
					case OUTPUT_PIPELINE:
						title << ": " << "Output";
						break;
					case SOURCE_PIPELINE:
						title << ": " << "Source";
						break;
					case DESTINATION_PIPELINE:
						title << ": " << "Destination";
						break;
					case NULLINPUTOUTPUT_PIPELINE:
						title << ": " << "Input/Output";
						break;
					case _NUM_PIPELINE:
						// shut the compiler up.
						break;
				}
#endif

	SetTitle(title.String() );
}

SeqFilterPropertyWindow::~SeqFilterPropertyWindow()
{
	if (mFilter->Filter() ) mFilter->Filter()->ConfigWindowClosed();
	mFilter->DecRefs();
	mFilter = NULL;
}

void SeqFilterPropertyWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_UNDO: {
			// WRITE MATRIX BLOCK
			AmPipelineMatrixI*	matrix = mMatrixRef.WriteLock();
			if (matrix && matrix->UndoContext() ) {
				BList context;
				context.AddItem(mFilter);
				matrix->UndoContext()->Undo(&context);
			}
			mMatrixRef.WriteUnlock(matrix);
			// END WRITE MATRIX BLOCK
		} break;
		
		case 'REDO': {
			// WRITE MATRIX BLOCK
			AmPipelineMatrixI*	matrix = mMatrixRef.WriteLock();
			if (matrix && matrix->UndoContext() ) {
				BList context;
				context.AddItem(mFilter);
				matrix->UndoContext()->Redo(&context);
			}
			mMatrixRef.WriteUnlock(matrix);
			// END WRITE MATRIX BLOCK
		} break;
		
		default:
			inherited::MessageReceived(msg);
	}
}

void SeqFilterPropertyWindow::AddPages(BColumnListView* list, BPoint& pt)
{
	ArpASSERT(list);
	pt.x = 0;
	pt.y = 0;
	if (!mFilter || !mFilter->Filter() ) return;

	BWindow*			win = new BWindow(BRect(0, 0, 2, 2), "NO", B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0);
	AmMultiFilter*		mf = dynamic_cast<AmMultiFilter*>(mFilter->Filter() );
	if (mf) AddMultiFilterPage(mf, list, NULL, pt, win);
	else AddFilterPage(mFilter->Filter(), list, NULL, pt, win);
	if (win) win->Quit();
}

// #pragma mark -

/********************************************************
 * _FILTER-PAGE-LIST-VIEW
 ********************************************************/
_FilterPageListView::_FilterPageListView(	BRect rect, const char *name,
											SeqAbstractFilterPropertyWindow* window)
		: inherited(rect, name, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER),
		  mWindow(window)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortingEnabled(false);
}

void _FilterPageListView::SelectionChanged()
{
	if (!mWindow) return;
	BView*				v = NULL;
	_FilterPageRow* 	row = dynamic_cast<_FilterPageRow*>(CurrentSelection() );
	if (row) v = row->View();
	mWindow->SetPage(v);
}

// #pragma mark -

/********************************************************
 * _FILTER-PAGE-ROW
 ********************************************************/
_FilterPageRow::_FilterPageRow(	const char* name, BView* view,
								float width)
		: mView(view), mWidth(width)
{
	if (name) SetField(new BStringField(name), NAME_COL_INDEX);
}

_FilterPageRow::~_FilterPageRow()
{
	if (mView) delete mView;
}

bool _FilterPageRow::HasLatch() const
{
	if (mView) return false;
	else return true;
}

BView* _FilterPageRow::View() const
{
	return mView;
}

float _FilterPageRow::Width() const
{
	return mWidth;
}
