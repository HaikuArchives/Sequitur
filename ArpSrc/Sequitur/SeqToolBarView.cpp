/* SeqToolBarView.cpp
 */
#include <cstdio>
#include <app/Application.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolBar.h"
#include "Sequitur/SeqFilterPropertyWindows.h"
#include "Sequitur/SeqToolBarView.h"
#include "Sequitur/SequiturDefs.h"

static const BBitmap*	gControlBg = NULL;
static const BBitmap*	gNormalIcon = NULL;
static const BBitmap*	gOverIcon = NULL;
static const BBitmap*	gPressedIcon = NULL;
static const BBitmap*	gPropertyMenuBm = NULL;

static const char*		EMPTY_STR = "Drop tools here";

static const uint32		PROPERTIES_MSG				= 'iPrp';
static const uint32		EDIT_MSG					= 'iEdt';
static const uint32		REMOVE_FROM_TOOL_BAR_MSG	= 'iRmv';

static const float		LAST_BUTTON_PAD				= 10;

static BRect invalid_rect()
{
	return BRect(-1, -1, -1, -1);
}

static bool is_valid(const BRect& r)
{
	return r.left != -1 && r.top != -1 && r.right != -1 && r.bottom != -1;
}

static BRect merge_rects(BRect r1, BRect r2)
{
	if (!is_valid(r1)) return r2;
	else if (!is_valid(r2)) return r1;
	else return r1 | r2;
}

static void init_constants()
{
	if (!gControlBg) gControlBg = ImageManager().FindBitmap(AM_TRACK_CONTROL_BG_STR);
	if (!gNormalIcon) gNormalIcon = ImageManager().FindBitmap(AM_TOOL_BORDER_NORMAL);
	if (!gOverIcon) gOverIcon = ImageManager().FindBitmap(AM_TOOL_BORDER_OVER);
	if (!gPressedIcon) gPressedIcon = ImageManager().FindBitmap(AM_TOOL_BORDER_PRESSED);
	if (!gPropertyMenuBm) gPropertyMenuBm = ImageManager().FindBitmap(SEQ_PROPERTY_MENU_NORMAL_STR);
}

/**********************************************************************
 * _SEQ-CACHED-TOOL
 **********************************************************************/
class _SeqCachedTool
{
public:
	_SeqCachedTool();
	_SeqCachedTool(const AmTool* tool);
	_SeqCachedTool(const _SeqCachedTool& o);
	virtual ~_SeqCachedTool();
	
	_SeqCachedTool&		operator=(const _SeqCachedTool& o);

	AmToolRef			ToolRef() const;

	float				DrawOn(	BView* view, BRect clip, float atX,
								BPoint pt);

	BRect				Bounds() const;

	BString			mToolKey;
	BString			mToolLabel;
	BString			mToolTip;
	BBitmap*		mNormalIcon;
	uint32			mButton;
	bool			mReadOnly;
	
	BPoint			mPropMenuOrigin;	// The x and y pixels to inset the property menu
										// by -- i.e., it's left top point.  This accounts
										// for the tool border.
};

/*************************************************************************
 * SEQ-TOOL-BAR-VIEW
 *************************************************************************/
SeqToolBarView::SeqToolBarView(	BPoint at, const char* name,
								const BString& toolBarName, float space)
		: inherited(BRect(at, at), name,
					B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  BToolTipable(*(BView*)this),
		  mToolBarName(toolBarName), mMouseDownTool(NULL), mOverToolIndex(-1),
		  mSpace(space), mPrefW(0), mPrefH(0)
{
	init_constants();
	mDraggingRect = invalid_rect();

	SetToolBar(AmGlobals().FindToolBar(toolBarName) );
}

SeqToolBarView::SeqToolBarView(	BPoint at, const char* name,
								AmToolBarRef toolBarRef, float space)
		: inherited(BRect(at, at), name,
					B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  BToolTipable(*(BView*)this),
		  mMouseDownTool(NULL), mOverToolIndex(-1), mSpace(space), mPrefW(0), mPrefH(0)
{
	init_constants();
	mDraggingRect = invalid_rect();

	SetToolBar(toolBarRef);
}

SeqToolBarView::~SeqToolBarView()
{
	if (mToolBarRef.IsValid() ) mToolBarRef.RemoveObserver(this);
	FreeCachedInfo();
}

void SeqToolBarView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor( Prefs().Color(AM_CONTROL_BG_C) );
	if (mToolBarRef.IsValid() ) mToolBarRef.AddObserver(this);
}

void SeqToolBarView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqToolBarView::GetPreferredSize(float *width, float *height)
{
	*width = mPrefW;
	*height = mPrefH;
}

void SeqToolBarView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case AmToolBar::TOOL_BAR_OBS: {
			CacheDrawingInfo();
			int32	action;
			/* If a tool was just replaced (i.e. AM_CHANGED),
			 * I can get away with just redrawing myself.
			 * Otherwise, my window needs to know about the change.
			 * This is a bit hacky.
			 */
			if (msg->FindInt32("action", &action) == B_OK
					&& action != AM_CHANGED) {
				Window()->PostMessage(AmGlobalsI::TOOL_BAR_OBS);
			} else Invalidate();
		} break;
		case PROPERTIES_MSG: {
			const char*			toolKey;
			AmToolRef			toolRef;
			AmToolRoster*		roster = AmToolRoster::Default();
			if (msg->FindString("tool_key", &toolKey) == B_OK
					&& roster && (toolRef = roster->FindTool(toolKey)).IsValid() ) {
				// READ TOOL LOCK
				BString			n;
				const AmTool*	tool = toolRef.ReadLock();
				if (tool) n = tool->Label();
				toolRef.ReadUnlock(tool);
				// END READ TOOL BLOCK
				if (n.Length() > 0) n << " ";
				n << "Tool";
				BWindow*		win = new SeqPipelinePropertyWindow(AmPipelineMatrixRef(toolRef),
																	NULLINPUTOUTPUT_PIPELINE,
																	n.String(), Window() );
				if (win) win->Show();
			}
		} break;
		case EDIT_MSG: {
			const char*		toolKey;
			const char*		path;
			if (msg->FindString("tool_key", &toolKey) == B_OK
					&& msg->FindString("path", &path) == B_OK) {
				BMessage	edt(SHOW_EDIT_TOOL_MSG);
				edt.AddString("tool_key", toolKey);
				edt.AddString("path", path);
				be_app->PostMessage(&edt);
			}
		} break;
		case REMOVE_FROM_TOOL_BAR_MSG: {
			const char*		toolKey;
			if (msg->FindString("tool_key", &toolKey) == B_OK) {
				BString		str(toolKey);
				// WRITE TOOL BAR LOCK
				AmToolBar*		toolBar = mToolBarRef.WriteLock();
				if (toolBar) toolBar->RemoveTool(str);
				mToolBarRef.WriteUnlock(toolBar);
				// END WRITE TOOL BAR BLOCK
			}
		} break;
		case AM_DRAG_TOOL_MSG: {
			const char*		toolName;
			if (msg->FindString("tool_name", &toolName) == B_OK)
				HandleToolDrop(toolName);
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void SeqToolBarView::MouseDown(BPoint where)
{
	AM_LOG("\nSeqToolBarView::MouseDown() 1\n");
	mDraggingRect = invalid_rect();
	mMouseDownButtonHack = 0;
	for (uint32 k = 0; k < mCachedData.size(); k++) mCachedData[k].mButton = 0;
	bool				hitPropertyMenu;
	mMouseDownTool = CachedToolAt(where, &hitPropertyMenu);
	if (mMouseDownTool) {
		AM_LOG("SeqToolBarView::MouseDown() 2\n");
		if (hitPropertyMenu) {
			AM_LOG("SeqToolBarView::MouseDown() 2a\n");
			// READ TOOL BLOCK
			AmToolRef		toolRef = mMouseDownTool->ToolRef();
			const AmTool*	tool = toolRef.ReadLock();
			if (tool) ShowPropertyMenu(tool, where);
			toolRef.ReadUnlock(tool);
			// END READ TOOL BLOCK
		} else {
			AM_LOG("SeqToolBarView::MouseDown() 2b\n");
			uint32			button = 0;
			Window()->CurrentMessage()->FindInt32("buttons", (int32*)&button);
#ifdef AM_LOGGING
			BString			str("\tbuttons: ");
			str << button << "\n";
			AM_BLOG(str);
#endif
			mMouseDownTool->mButton = button;
			mMouseDownButtonHack = button;
			Invalidate();
		}
	}
	AM_LOG("SeqToolBarView::MouseDown() 3 (end)\n");
}

void SeqToolBarView::MouseMoved(BPoint where,
								uint32 code,
								const BMessage *a_message)
{
	AM_LOG("SeqToolBarView::MouseMoved()\n");
	/* Sometimes, this view might not receive a mouse up for
	 * whatever reason
	 */
	uint32			button = 0;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&button);
	if (button == 0) PostMouseUp();
#ifdef AM_LOGGING
	BString			str("\tbutton: ");
	str << button << "\n";
	AM_BLOG(str);
#endif

	BRect		invalidate = invalid_rect();
	BRect		b( Bounds() );
	/* If the dragging area has changed, merge it into the invalidate.
	 */
	BRect		oldDraggingRect(mDraggingRect);
	mDraggingRect = invalid_rect();
	if (b.Contains(where) && a_message && a_message->what == AM_DRAG_TOOL_MSG) {
		if (mCachedData.size() < 1) mDraggingRect = b;
		else mDraggingRect = DraggingRectFor(where);
	}
	if (oldDraggingRect != mDraggingRect) {
		invalidate = merge_rects(invalidate, oldDraggingRect);
		invalidate = merge_rects(invalidate, mDraggingRect);
	}
		
	Invalidate();
}

void SeqToolBarView::MouseUp(BPoint where)
{
	AM_LOG("\nSeqToolBarView::MouseUp() 1\n");
	if (mMouseDownTool) {
		AM_LOG("SeqToolBarView::MouseUp() 2\n");
		_SeqCachedTool*		cachedTool = CachedToolAt(where);
		if (mMouseDownTool == cachedTool) {
			AM_LOG("SeqToolBarView::MouseUp() 3\n");
			if (cachedTool->mToolKey.Length() > 0) {
				AM_LOG("SeqToolBarView::MouseUp() 4\n");
				uint32		button = 0;
				if (cachedTool->mButton&B_PRIMARY_MOUSE_BUTTON) button = B_PRIMARY_MOUSE_BUTTON;
				else if (cachedTool->mButton&B_SECONDARY_MOUSE_BUTTON) button = B_SECONDARY_MOUSE_BUTTON;
				else if (cachedTool->mButton&B_TERTIARY_MOUSE_BUTTON) button = B_TERTIARY_MOUSE_BUTTON;
				/* This is a hack because the WACOM graphics tablet driver seems
				 * to always report that no button is held down during mouse move.
				 */
				if (button == 0 && mMouseDownButtonHack != 0) {
					if (mMouseDownButtonHack&B_PRIMARY_MOUSE_BUTTON) button = B_PRIMARY_MOUSE_BUTTON;
					else if (mMouseDownButtonHack&B_SECONDARY_MOUSE_BUTTON) button = B_SECONDARY_MOUSE_BUTTON;
					else if (mMouseDownButtonHack&B_TERTIARY_MOUSE_BUTTON) button = B_TERTIARY_MOUSE_BUTTON;
				}
#ifdef AM_LOGGING
				BString			str("\tbutton: ");
				str << button << ", cachedTool button: " << cachedTool->mButton << "\n";
				AM_BLOG(str);
#endif
				if (button) AmGlobals().SetTool(cachedTool->mToolKey, button, modifiers());
			}
		}
	}
	PostMouseUp();
	mMouseDownButtonHack = 0;
	Invalidate();
	AM_LOG("SeqToolBarView::MouseUp() 5 (end)\n");
}

void SeqToolBarView::SetViewColor(rgb_color c)
{
	mViewC = c;
	inherited::SetViewColor(B_TRANSPARENT_COLOR);
}

status_t SeqToolBarView::GetToolTipInfo(BPoint where, BRect* out_region,
										BToolTipInfo* out_info)
{
	if (mCachedData.size() < 1) {
		*out_region = Bounds();
		if (out_info) out_info->SetText("Drop tools here");
	} else {
		bool		b;
		BRect		r;
		_SeqCachedTool*		cachedTool = CachedToolAt(where, &b, &r);
		if (!cachedTool) return B_ERROR;
		*out_region = r;
		if (out_info) out_info->SetText(cachedTool->mToolTip.String() );
	}
	return B_OK;
}

void SeqToolBarView::SetToolBar(AmToolBarRef toolBarRef)
{
	if (mToolBarRef.IsValid() ) mToolBarRef.RemoveObserver(this);
	mToolBarRef = toolBarRef;
	CacheDrawingInfo();
	Invalidate();
}

void SeqToolBarView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(mViewC);
	view->FillRect(clip);
	if (gControlBg) arp_tile_bitmap_on(view, clip, gControlBg, Frame().LeftTop() );

	if (mCachedData.size() < 1) {
		view->SetHighColor( Prefs().Color(AM_CONTROL_FG_C) );
		view->DrawString(EMPTY_STR, BPoint(4, 4 + arp_get_font_height(this)) );
	}

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

#if 0
	if (mCachedData.size() >= 1 && mToolBarName.Length() > 0) {
		view->SetHighColor(0, 0, 0, 100);
		view->DrawString(mToolBarName.String(), BPoint(4, 4 + arp_get_font_height(this)) );
	}
#endif

	BPoint			pt(-1, -1);
	uint32			button;
	if (!is_valid(mDraggingRect)) GetMouse(&pt, &button, false);

	float			x  = 0;
	for (uint32 k = 0; k < mCachedData.size(); k++) {
		x = mCachedData[k].DrawOn(view, clip, x, pt);
		x += 1 + mSpace;
		if (x > clip.right) break;
	}
	if (is_valid(mDraggingRect)) {
		view->SetHighColor(0, 0, 0, 127);
		view->FillRect(mDraggingRect);
	}

	view->SetDrawingMode(mode);
}

void SeqToolBarView::PostMouseUp()
{
	AM_LOG("\nSeqToolBarView::PostMouseUp()\n");
	for (uint32 k = 0; k < mCachedData.size(); k++) mCachedData[k].mButton = 0;
	mDraggingRect = invalid_rect();
}

void SeqToolBarView::CacheDrawingInfo()
{
	mPrefW = 0;
	mPrefH = 0;

	FreeCachedInfo();	
	toolref_vec			toolRefs;
	FillVec(toolRefs);

	for (uint32 k = 0; k < toolRefs.size(); k++) {
		// READ TOOL BLOCK
		const AmTool*	tool = toolRefs[k].ReadLock();
		if (tool) {
			BRect		b = tool->IconBounds();
			mPrefW += b.Width() + 1 + mSpace;
			if (b.Height() > mPrefH) mPrefH = b.Height();
			mCachedData.push_back( _SeqCachedTool(tool) );
		}
		toolRefs[k].ReadUnlock(tool);
		// END READ TOOL BLOCK
	}
	/* Oops, I'm empty, prepare myself to just draw a string.
	 */
	if (mCachedData.size() < 1) {
		mPrefW = 4 + StringWidth(EMPTY_STR) + 4;
		mPrefH = 4 + arp_get_font_height(this) + 4;
		return;
	}
	/* Extra space at the end to make it easier to drop in tools.
	 */
	mPrefW += LAST_BUTTON_PAD;
}

void SeqToolBarView::FillVec(toolref_vec& toolRefs) const
{
	// READ TOOL BAR BLOCK
	const AmToolBar*	toolBar = mToolBarRef.ReadLock();
	if (toolBar) {
		mToolBarName = toolBar->Name();
		uint32			count = toolBar->CountTools();
		for (uint32 k = 0; k < count; k++) {
			AmToolRef		toolRef = toolBar->ToolAt(k);
			if (toolRef.IsValid() ) toolRefs.push_back(toolRef);
		}
	}
	mToolBarRef.ReadUnlock(toolBar);
	// END READ TOOL BAR BLOCK
}

void SeqToolBarView::FreeCachedInfo()
{
	mMouseDownTool = NULL;
	mCachedData.resize(0);
}

_SeqCachedTool* SeqToolBarView::CachedToolAt(BPoint pt, bool* hitPropertyMenu, BRect* frame)
{
	if (hitPropertyMenu) *hitPropertyMenu = false;
	float			x  = 0;
	for (uint32 k = 0; k < mCachedData.size(); k++) {
		BRect		b = mCachedData[k].Bounds();
		b.OffsetBy( BPoint(x, 0) );
		if (b.Contains(pt)) {
			if (hitPropertyMenu && gPropertyMenuBm) {
				BRect		propB(gPropertyMenuBm->Bounds() );
				propB.OffsetBy(mCachedData[k].mPropMenuOrigin);
				propB.OffsetBy( BPoint(x, 0) );
				if (propB.Contains(pt) ) *hitPropertyMenu = true;
			}
			if (frame) *frame = b;
			return &mCachedData[k];
		}
		x = b.right + 1;
		x += mSpace;
		if (x > pt.x) return NULL;
	}
	return NULL;
}

BRect SeqToolBarView::DraggingRectFor(BPoint pt, int32* lIndex, int32* rIndex)
{
	if (mCachedData.size() < 1) {
		if (lIndex) *lIndex = -1;
		if (rIndex) *rIndex = 0;
		return Bounds();
	}
	BRect			bounds(Bounds() );
	float			x  = 0;
	float			IN_BETWEEN = 0.20;

	for (uint32 k = 0; k < mCachedData.size(); k++) {
		BRect		b = mCachedData[k].Bounds();
		float		inBetween = b.Width() * IN_BETWEEN;
		float		pad = 0;
		b.OffsetBy(BPoint(x, 0) );
		/* Special case:  If we're on the last tool, the drag bounds
		 * extend to the end of the view.
		 */
		if (k >= mCachedData.size() - 1) {
			b.right = bounds.right;
			pad = LAST_BUTTON_PAD;
		}

		if (b.Contains(pt)) {
			if (lIndex && rIndex) *lIndex = *rIndex = (int32)k;
			BRect		ans(b);
			if (pt.x <= b.left + inBetween) {
				if (lIndex) *lIndex = *lIndex -1;
				ans.right = ans.left + inBetween;
				if (k > 0) {
					float	f = (mCachedData[k-1].Bounds().Width()) * IN_BETWEEN;
					ans.left -= (mSpace + f);
				}
			} else if (pt.x >= b.right - inBetween - pad) {
				if (rIndex) *rIndex = *rIndex + 1;
				ans.left = ans.right - inBetween - pad;
				if (k != mCachedData.size() - 1) {
					float	f = (mCachedData[k+1].Bounds().Width()) * IN_BETWEEN;
					ans.right += mSpace + f;
				}
			}
			return ans;
		}
		x = b.right + 1;
		x += mSpace;
		if (x > pt.x) return BRect(-1, -1, -1, -1);
	}
	return BRect(-1, -1, -1, -1);
}

void SeqToolBarView::HandleToolDrop(const char* toolName)
{
	BPoint			pt(0, 0);
	uint32			button;
	GetMouse(&pt, &button, false);
	int32			lIndex, rIndex;
	BRect			r = DraggingRectFor(pt, &lIndex, &rIndex);
	if (is_valid(r)) {
		AmToolRef	toolRef = AmGlobals().FindTool(toolName);
		if (toolRef.IsValid() ) {
			// WRITE TOOL BAR BLOCK
			AmToolBar*	toolBar = mToolBarRef.WriteLock();
			if (toolBar) {
				if (lIndex == rIndex) toolBar->ReplaceTool(toolRef.ToolKey(), lIndex);
				else toolBar->InsertTool(toolRef.ToolKey(), rIndex);
			}
			mToolBarRef.WriteUnlock(toolBar);
			// END WRITE TOOL BLOCK
		}
	}
}

void SeqToolBarView::ShowPropertyMenu(const AmTool* tool, BPoint where)
{
	BPopUpMenu*			menu = new BPopUpMenu("properties menu");
	if (!menu) return;
	BMessage*			msg = new BMessage(PROPERTIES_MSG);
	if (msg) {
		msg->AddString("tool_key", tool->Key() );
		BMenuItem*		item = new BMenuItem("Properties", msg);
		if (item) {
			item->SetTarget(this);
			menu->AddItem(item);
		}
	}
	msg = new BMessage(EDIT_MSG);
	if (msg) {
		msg->AddString("tool_key", tool->Key() );
		msg->AddString("path", tool->LocalFilePath() );
		BMenuItem*		item = new BMenuItem("Edit", msg);
		if (item) {
			item->SetTarget(this);
			if (tool->IsReadOnly() ) item->SetEnabled(false);
			menu->AddItem(item);
		}
	}

	menu->AddSeparatorItem();

	msg = new BMessage(REMOVE_FROM_TOOL_BAR_MSG);
	if (msg) {
		msg->AddString("tool_key", tool->Key() );
		msg->AddPointer("tool_id", tool->Id() );
		BMenuItem*		item = new BMenuItem("Remove from tool bar", msg);
		if (item) {
			item->SetTarget(this);
			menu->AddItem(item);
		}
	}

	menu->SetAsyncAutoDestruct(true);
	where = ConvertToScreen(where);
	BRect	sticky(where.x-5, where.y-5, where.x+5, where.y+5);
	menu->Go(where, true, false, sticky, true);
}

// #pragma mark -

/**********************************************************************
 * _SEQ-CACHED-TOOL
 **********************************************************************/
_SeqCachedTool::_SeqCachedTool()
		: mNormalIcon(NULL), mButton(0), mReadOnly(false), mPropMenuOrigin(0, 0)
{
}

_SeqCachedTool::_SeqCachedTool(const AmTool* tool)
		: mNormalIcon(NULL), mButton(0), mReadOnly(false), mPropMenuOrigin(0, 0)
{
	ArpASSERT(tool);
	mToolKey = tool->Key();
	mToolLabel = tool->Label();
	mToolTip = tool->ToolTip();
	const BBitmap*		icon = tool->Icon();
	if (icon) mNormalIcon = new BBitmap(icon);
	mReadOnly = tool->IsReadOnly();
}

_SeqCachedTool::_SeqCachedTool(const _SeqCachedTool& o)
		: mToolKey(o.mToolKey), mToolLabel(o.mToolLabel), mToolTip(o.mToolTip),
		  mButton(o.mButton), mPropMenuOrigin(o.mPropMenuOrigin)
{
	mNormalIcon = new BBitmap(o.mNormalIcon);
}

_SeqCachedTool::~_SeqCachedTool()
{
	delete mNormalIcon;
}

_SeqCachedTool& _SeqCachedTool::operator=(const _SeqCachedTool &o)
{
	mToolKey = o.mToolKey;
	mToolLabel = o.mToolLabel;
	mToolTip = o.mToolTip;
	mNormalIcon = new BBitmap(o.mNormalIcon);
	mButton = o.mButton;
	mPropMenuOrigin = o.mPropMenuOrigin;
	return *this;
}

AmToolRef _SeqCachedTool::ToolRef() const
{
	AmToolRoster*	roster = AmToolRoster::Default();
	if (!roster) return AmToolRef();
	return roster->FindTool(mToolKey.String() );
}

float _SeqCachedTool::DrawOn(	BView* view, BRect clip, float atX,
								BPoint pt)
{
	BBitmap*		icon = mNormalIcon;
	const BBitmap*	globalIcon  = gNormalIcon;
	const BBitmap*	menuBm = NULL;
	bool			pressed = false;
	if (!icon) return atX;
	BRect			r(atX, 0, atX + icon->Bounds().Width(), icon->Bounds().Height());
	if (clip.right < r.left) return atX + r.Width();
	if (r.Contains(pt) ) {
		if (mButton&B_PRIMARY_MOUSE_BUTTON || mButton&B_SECONDARY_MOUSE_BUTTON || mButton&B_TERTIARY_MOUSE_BUTTON) {
			globalIcon = gPressedIcon;
			pressed = true;
		} else {
			globalIcon = gOverIcon;
			menuBm = gPropertyMenuBm;
		}
	}
	BPoint	origin(atX, 0);
	if (icon) {
		if (pressed) {
			BRect	src(icon->Bounds() );
			src.right--;
			src.bottom--;
			BRect	dest(src);
			dest.OffsetBy(BPoint(origin.x + 1, origin.y + 1));
			view->DrawBitmapAsync(icon, src, dest);
		} else view->DrawBitmapAsync(icon, origin);
	}
	if (globalIcon) view->DrawBitmapAsync(globalIcon, origin);
	if (menuBm) view->DrawBitmapAsync(menuBm, BPoint(origin.x + mPropMenuOrigin.x, origin.y + mPropMenuOrigin.y) );
	return origin.x + r.Width();
}

BRect _SeqCachedTool::Bounds() const
{
	if (mNormalIcon) return mNormalIcon->Bounds();
	return BRect(0, 0, 0, 0);
}
