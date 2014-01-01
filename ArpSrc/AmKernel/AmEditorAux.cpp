/* AmEditorAux.cpp
 */
#include <stdio.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmDefs.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmKernel/AmEditorAux.h"

/*************************************************************************
 * AM-EDITOR-TOOL
 *************************************************************************/
AmEditorTool::AmEditorTool(	const BBitmap* icon, const BString& toolTip,
							uint32 code)
		: mIcon(icon), mToolTip(toolTip), mCode(code)
{
}

const BBitmap* AmEditorTool::Icon() const
{
	return mIcon;
}

BString AmEditorTool::ToolTip() const
{
	return mToolTip;
}

uint32 AmEditorTool::Code() const
{
	return mCode;
}

/*************************************************************************
 * AM-ACTIVE-TOOL-VIEW
 *************************************************************************/
AmActiveToolView::AmActiveToolView(	BPoint origin, float leftOverhang,
										float topOverhang, float rightOverhang)
		: inherited(BRect(origin, origin), "active_tools", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  mMouseType(1), mMouseBitmap(NULL), mPrimaryTool(NULL), mSecondaryTool(NULL),
		  mTertiaryTool(NULL), mLeftOverhang(leftOverhang),
		  mRightOverhang(rightOverhang), mTopOverhang(topOverhang)
{
	if (get_mouse_type(&mMouseType) != B_OK) mMouseType = 1;
	/* Get my mouse image based on the mouse type.
	 */	
	if (mMouseType == 1) {
		mMouseBitmap = ImageManager().FindBitmap("Mouse 1");
	} else if (mMouseType == 2) {
		mMouseBitmap = ImageManager().FindBitmap("Mouse 2");
	} else if (mMouseType == 3) {
		mMouseBitmap = ImageManager().FindBitmap("Mouse 3");
	}
}

void AmActiveToolView::SetActiveTool(int32 button, const AmEditorTool* tool)
{
	if (button&B_PRIMARY_MOUSE_BUTTON) mPrimaryTool = tool;
	else if (button&B_SECONDARY_MOUSE_BUTTON) mSecondaryTool = tool;
	else if (button&B_TERTIARY_MOUSE_BUTTON) mTertiaryTool = tool;
	Invalidate();
}

const AmEditorTool* AmActiveToolView::ActiveTool(int32 button) const
{
	if (button&B_PRIMARY_MOUSE_BUTTON) return mPrimaryTool;
	else if (button&B_SECONDARY_MOUSE_BUTTON) return mSecondaryTool;
	else if (button&B_TERTIARY_MOUSE_BUTTON) return mTertiaryTool;
	else return NULL;
}

uint32 AmActiveToolView::ActiveToolCode(int32 button) const
{
	const AmEditorTool*		tool = ActiveTool(button);
	if (!tool) return 0;
	return tool->Code();
}

void AmActiveToolView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	if (Parent() ) mViewC = Parent()->ViewColor();
}

void AmActiveToolView::Draw(BRect clip)
{
	BView* into = this;
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void AmActiveToolView::GetPreferredSize(float* width, float* height)
{
	float	w = mLeftOverhang + mRightOverhang;
	float	h = mTopOverhang;
	if (mMouseBitmap) {
		BRect	b( mMouseBitmap->Bounds() );
		w += b.Width();
		h += b.Height();
	}
	*width = w;
	*height = h;
}

void AmActiveToolView::ClearTools()
{
	mPrimaryTool = mSecondaryTool = mTertiaryTool = NULL;
}

void AmActiveToolView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(mViewC);
	view->FillRect(clip);

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	if (mMouseBitmap) view->DrawBitmapAsync(mMouseBitmap, BPoint(mLeftOverhang, mTopOverhang) );
	/* Draw the tools.
	 */
	if (mMouseType == 1) {
		if (mPrimaryTool && mPrimaryTool->Icon() ) view->DrawBitmapAsync(mPrimaryTool->Icon(), BPoint(15, 2) );
	} else if (mMouseType == 2) {
		if (mPrimaryTool && mPrimaryTool->Icon() ) view->DrawBitmapAsync(mPrimaryTool->Icon(), BPoint(4, 2) );
		if (mSecondaryTool && mSecondaryTool->Icon() ) view->DrawBitmapAsync(mSecondaryTool->Icon(), BPoint(28, 2) );
	} else if (mMouseType == 3) {
		if (mPrimaryTool && mPrimaryTool->Icon() ) view->DrawBitmapAsync(mPrimaryTool->Icon(), BPoint(0, 2) );
		if (mTertiaryTool && mTertiaryTool->Icon() ) view->DrawBitmapAsync(mTertiaryTool->Icon(), BPoint(24, 0) );
		if (mSecondaryTool && mSecondaryTool->Icon() ) view->DrawBitmapAsync(mSecondaryTool->Icon(), BPoint(48, 2) );
	}
	
	SetDrawingMode(mode);
}

/*************************************************************************
 * AM-EDITOR-TOOL-BAR-VIEW
 *************************************************************************/
static const BBitmap*	gNormalIcon = NULL;
static const BBitmap*	gOverIcon = NULL;
static const BBitmap*	gPressedIcon = NULL;

AmEditorToolBarView::AmEditorToolBarView(BPoint at, const char* name, float space)
		: inherited(BRect(at, at), name,
					B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  BToolTipable(*(BView*)this),
		  mMouseDownTool(NULL), mMouseDownButton(0), mMouseOverTool(NULL),
		  mActiveToolView(NULL),
		  mSpace(space), mPrefW(0), mPrefH(0)
{
	if (!gNormalIcon) gNormalIcon = ImageManager().FindBitmap(AM_TOOL_BORDER_NORMAL);
	if (!gOverIcon) gOverIcon = ImageManager().FindBitmap(AM_TOOL_BORDER_OVER);
	if (!gPressedIcon) gPressedIcon = ImageManager().FindBitmap(AM_TOOL_BORDER_PRESSED);
}

AmEditorToolBarView::~AmEditorToolBarView()
{
}

void AmEditorToolBarView::AddTool(AmEditorTool* tool)
{
	mTools.push_back(tool);
	mPrefW = mPrefH = 0;
	for (uint32 k = 0; k < mTools.size(); k++) {
		if (mTools[k] && mTools[k]->Icon() ) {
			BRect		b(mTools[k]->Icon()->Bounds());
			mPrefW += b.Width() + 1;
			if (b.Height() > mPrefH) mPrefH = b.Height();
		}
	}
}

void AmEditorToolBarView::SetActiveToolView(AmActiveToolView* atv)
{
	mActiveToolView = atv;
}

void AmEditorToolBarView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	if (Parent() ) mViewC = Parent()->ViewColor();
}

void AmEditorToolBarView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void AmEditorToolBarView::GetPreferredSize(float *width, float *height)
{
	*width = mPrefW;
	*height = mPrefH;
}

void AmEditorToolBarView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		default:
			inherited::MessageReceived(msg);
	}
}

void AmEditorToolBarView::MouseDown(BPoint where)
{
	BRect			invalid = arp_invalid_rect();
	BRect			frame;
	mMouseDownTool = ToolAt(where, &frame);
	if (mMouseDownTool) {
		invalid = arp_merge_rects(invalid, frame);
		mMouseDownButton = 0;
		Window()->CurrentMessage()->FindInt32("buttons", (int32*)&mMouseDownButton);
		if (mMouseOverTool && mMouseOverTool != mMouseDownTool)
			invalid = arp_merge_rects(invalid, Bounds() );
		mMouseOverTool = NULL;
	}

	if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
}

void AmEditorToolBarView::MouseMoved(	BPoint where,
										uint32 code,
										const BMessage *a_message)
{
	/* Sometimes, this view might not receive a mouse up for
	 * whatever reason, which can cause the scroll runner to
	 * lock into permanent autoscroll.  This simply makes sure
	 * the scroll runner has been deleted if it's not valid.
	 */
	int32			button = 0;
	Window()->CurrentMessage()->FindInt32("buttons", &button);
	if (button == 0) PostMouseUp();

	BRect			invalid = arp_invalid_rect();
	AmEditorTool*	tool = ToolAt(where);
	if (tool != mMouseOverTool) {
		invalid = arp_merge_rects(invalid, Bounds() );
		mMouseOverTool = tool;
	}

	if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
}

void AmEditorToolBarView::MouseUp(BPoint where)
{
	if (mActiveToolView) {
		AmEditorTool*		tool = ToolAt(where);
		if (tool == mMouseDownTool)
			mActiveToolView->SetActiveTool(mMouseDownButton, tool);		
	}
	PostMouseUp();
}

status_t AmEditorToolBarView::GetToolTipInfo(	BPoint where, BRect* out_region,
												BToolTipInfo* out_info)
{
	BRect			f;
	AmEditorTool*	tool = ToolAt(where, &f);
	if (!tool) return B_ERROR;
	*out_region = f;
	if (out_info) out_info->SetText(tool->ToolTip().String() );
	return B_OK;
}

void AmEditorToolBarView::ClearTools()
{
	mTools.resize(0);
	mMouseDownTool = NULL;
	mMouseOverTool = NULL;
}

void AmEditorToolBarView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(mViewC);
	view->FillRect(clip);

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	BPoint			mousePt;
	uint32			button;
	GetMouse(&mousePt, &button, false);

	float			x  = 0;
	for (uint32 k = 0; k < mTools.size(); k++) {
		x = DrawToolOn(view, clip, mTools[k], x, mousePt);
		x += 1 + mSpace;
		if (x > clip.right) break;
	}

	view->SetDrawingMode(mode);
}

void AmEditorToolBarView::PostMouseUp()
{
	mMouseDownTool = NULL;
	mMouseDownButton = 0;
}

float AmEditorToolBarView::DrawToolOn(	BView* view, BRect clip, AmEditorTool* tool,
										float atX, BPoint mousePt)
{
	ArpASSERT(tool && tool->Icon() );
	if (!tool || !tool->Icon() ) return atX;
	const BBitmap*	icon = tool->Icon();
	const BBitmap*	globalIcon = NULL;
	BRect			r(atX, 0, atX + icon->Bounds().Width(), icon->Bounds().Height());
	if (clip.right < r.left) return atX + r.Width();
	if (r.Contains(mousePt) ) {
		if (tool == mMouseDownTool) {
			globalIcon = gPressedIcon;
		} else {
			globalIcon = gOverIcon;
		}
	}
	BPoint	origin(atX, 0);
	if (tool == mMouseDownTool) {
		BRect	src(icon->Bounds() );
		src.right--;
		src.bottom--;
		BRect	dest(src);
		dest.OffsetBy(BPoint(origin.x + 1, origin.y + 1));
		view->DrawBitmapAsync(icon, src, dest);
	} else view->DrawBitmapAsync(icon, origin);

	if (globalIcon) view->DrawBitmapAsync(globalIcon, origin);
	return origin.x + r.Width();
}

AmEditorTool* AmEditorToolBarView::ToolAt(BPoint where, BRect* frame)
{
	float			x  = 0;
	for (uint32 k = 0; k < mTools.size(); k++) {
		if (mTools[k] && mTools[k]->Icon() ) {
			BRect		b = mTools[k]->Icon()->Bounds();
			b.OffsetBy( BPoint(x, 0) );
			if (b.Contains(where)) {
				if (frame) *frame = b;
				return mTools[k];
			}
			x = b.right + 1;
			x += mSpace;
			if (x > where.x) return NULL;
		}
	}
	return NULL;
}
