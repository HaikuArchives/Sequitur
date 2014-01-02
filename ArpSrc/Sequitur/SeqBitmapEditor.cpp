/* SeqBitmapEditor.cpp
 */
#include <stdio.h>
//#include <experimental/BitmapTools.h>
#include <app/Clipboard.h>
#include <interface/ColorControl.h>
#include <interface/ScrollView.h>
#include <interface/Window.h>
#include <storage/Entry.h>
#include <translation/TranslationUtils.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpLineArrayCache.h"
#include "ArpViews/ArpKnobControl.h"
#include "ArpViews/ArpRangeControl.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmEditorAux.h"
#include "Sequitur/SeqBitmapEditor.h"

static const char*		ICON_VIEW_STR		= "icon_view";

const BBitmap*			gPencilIcon = NULL;
const BBitmap*			gDropperIcon = NULL;

static const uint32		PENCIL_TOOL_CODE		= 1;
static const uint32		DROPPER_TOOL_CODE		= 2;

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
//static rgb_color	read_pixel(const BBitmap* bm, float x, float y, pixel_access& pa);
//static void			write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa);
static rgb_color	change_color(rgb_color c1, int32 change);
static inline bool	colors_equal(rgb_color c1, rgb_color c2);

/*************************************************************************
 * ICON-DROP-VIEW
 *************************************************************************/
class IconDropView : public BView
{
public:
	IconDropView(BRect frame, BBitmap* bitmap, AmActiveToolView* activeTools);
	virtual ~IconDropView();

	void			SetBitmap(BBitmap* bitmap);
	void			SetBitmapChangeMessage(BMessage* msg);	
	void			SetControls(BColorControl* colorCtrl, ArpKnobControl* knobCtrl);
	void			SetZoom(float zoom);
	
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect clip);
	virtual	void	GetPreferredSize(float *width, float *height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage *a_message);

	/* Manipulations.
	 */
	status_t		Copy();
	status_t		Paste();
	status_t		FlipVertically();
	status_t		FlipHorizontally();
	status_t		FillAlpha();

private:
	typedef BView			inherited;
	BBitmap*				mBitmap;
	AmActiveToolView*		mActiveTools;
	BColorControl*			mColorCtrl;
	ArpKnobControl*			mKnobCtrl;
	float					mPixelSize;
	rgb_color				mBgC;
	bool					mHasChanges;
	BMessage*				mBitmapChangeMsg;
	//pixel_access			mPixelAccess;
	
	void			DrawOn(BRect clip, BView* view);
	void			HandleRefDrop(BMessage* msg);
	void			WritePixel(BPoint where);
	void			ReadPixel(BPoint where);
	BPoint			PixelAt(BPoint where) const;
	rgb_color		CurrentColor();
	void			SetHasChanges(bool hasChanges);
};

/*************************************************************************
 * SEQ-BITMAP-EDITOR
 *************************************************************************/
SeqBitmapEditor::SeqBitmapEditor(	BRect frame, const char* name,
									BBitmap* bitmap, uint32 resizeMask,
									BMenu* actionMenu)
		: inherited(frame, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
		  mActiveTools(NULL), mToolBar(NULL), mPencilTool(NULL),
		  mDropperTool(NULL), mScrollView(NULL)
{
	if (!gPencilIcon) gPencilIcon = ImageManager().FindBitmap("Pencil Tool");
	if (!gDropperIcon) gDropperIcon = ImageManager().FindBitmap("Dropper Tool");

	AddViews(Bounds(), actionMenu);
	SetBitmap(bitmap);
}

SeqBitmapEditor::~SeqBitmapEditor()
{
}

void SeqBitmapEditor::AttachedToWindow()
{
	inherited::AttachedToWindow();
}

void SeqBitmapEditor::FrameResized(float new_width, float new_height)
{
	inherited::AttachedToWindow();
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (v) SetupScrollBars(v);
}

void SeqBitmapEditor::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			int32		what = 0;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
			if (what == 'ibez') HandleZoom(msg);
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void SeqBitmapEditor::SetBitmap(BBitmap* bitmap)
{
	if (Window() && !Window()->Lock() ) return;
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (v) v->SetBitmap(bitmap);
	if (Window() ) Window()->Unlock();
}

void SeqBitmapEditor::SetBitmapChangeMessage(BMessage* msg)
{
	if (Window() && !Window()->Lock() ) return;
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (v) v->SetBitmapChangeMessage(msg);
	if (Window() ) Window()->Unlock();
}

status_t SeqBitmapEditor::Copy()
{
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (!v) return B_ERROR;
	return v->Copy();
}

status_t SeqBitmapEditor::Paste()
{
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (!v) return B_ERROR;
	return v->Paste();
}

status_t SeqBitmapEditor::FlipVertically()
{
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (!v) return B_ERROR;
	return v->FlipVertically();
}

status_t SeqBitmapEditor::FlipHorizontally()
{
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (!v) return B_ERROR;
	return v->FlipHorizontally();
}

status_t SeqBitmapEditor::FillAlpha()
{
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (!v) return B_ERROR;
	return v->FillAlpha();
}

void SeqBitmapEditor::HandleZoom(BMessage* msg)
{
	float			valueX;
	if (msg->FindFloat("x_value", &valueX) != B_OK) return;
	IconDropView*	v = dynamic_cast<IconDropView*>( FindView(ICON_VIEW_STR) );
	if (!v) return;
	v->SetZoom(valueX);
	SetupScrollBars(v);
}

void SeqBitmapEditor::SetupScrollBars(BView* target)
{
	if (!mScrollView) return;
	ArpVALIDATE(target, return);
	BScrollBar*	b = mScrollView->ScrollBar(B_HORIZONTAL);
	if (b) arp_setup_horizontal_scroll_bar(b, target);
	b = mScrollView->ScrollBar(B_VERTICAL);
	if (b) arp_setup_vertical_scroll_bar(b, target);
}

void SeqBitmapEditor::AddViews(BRect frame, BMenu* actionMenu)
{
	/* Add the Action menu field.
	 */
	float			w = 0;
	if (actionMenu) {
		w = StringWidth(actionMenu->Name() ) + 28;
		BRect			actionR(0, 0, w, 10);
		BMenuField*		field = new BMenuField(actionR, "action_fld", NULL, actionMenu);
		if (field) AddChild(field);
		else delete actionMenu;
	}
	/* Add the tool row.
	 */
	float			left = frame.left;
	float			top = frame.top;
	mActiveTools = new AmActiveToolView(BPoint(w + 5, 0) );
	if (mActiveTools) {
		mActiveTools->ResizeToPreferred();
		top = mActiveTools->Frame().bottom;
		left = mActiveTools->Frame().right + 5;
		AddChild(mActiveTools);
	}
	mToolBar = new AmEditorToolBarView(BPoint(left, 4), "toolbar");
	if (mToolBar) {
		if (gPencilIcon) mPencilTool = new AmEditorTool(gPencilIcon, "Pencil", PENCIL_TOOL_CODE);
		if (mPencilTool) mToolBar->AddTool(mPencilTool);
		if (gDropperIcon) mDropperTool = new AmEditorTool(gDropperIcon, "Dropper", DROPPER_TOOL_CODE);
		if (mDropperTool) mToolBar->AddTool(mDropperTool);

		mToolBar->SetActiveToolView(mActiveTools);
		mToolBar->ResizeToPreferred();
		BRect		f = mToolBar->Frame();
		if (f.bottom > top) top = f.bottom;
		left = f.right + 5;
		AddChild(mToolBar);
	}
	if (mActiveTools) {
		mActiveTools->SetActiveTool(B_PRIMARY_MOUSE_BUTTON, mPencilTool);
		mActiveTools->SetActiveTool(B_SECONDARY_MOUSE_BUTTON, mDropperTool);
	}
	/* Add the editing view.
	 */
	frame.top = top + 5;
	frame.bottom = frame.bottom - 50;
	BRect		r(frame);
	r.left += 2;
	r.bottom -= 50;
	r.right -= (Prefs().Size(V_SCROLLBAR_X) + 4);
	IconDropView*		iconView = new IconDropView(r, NULL, mActiveTools);
	BView*				addTo = iconView;
	if (iconView) {
		mScrollView = new BScrollView("scroll", iconView, B_FOLLOW_ALL, 0, true, true);	
		if (mScrollView) {
			AddChild(mScrollView);
			addTo = mScrollView;
		} else AddChild(iconView);
	}
	/* Add the zoom control.
	 */
	BRect		zoomF(r.Width() + 3, r.Height() + 3, r.Width() + 3 + ARP_ZOOM_CONTROL_WIDTH, r.Height() + 3 + ARP_ZOOM_CONTROL_HEIGHT);
	ArpRangeControl*	zoom = new ArpZoomControl(zoomF, "zoom");
	if (zoom) {
		zoom->AddHorizontalBand( 1,		1,		10 );
		zoom->AddHorizontalBand( 2,		2,		10 );
		zoom->AddHorizontalBand( 4,		4,		10 );
		zoom->AddHorizontalBand( 6,		6,		10 );
		zoom->AddHorizontalBand( 8,		8,		10 );
		zoom->AddHorizontalBand( 10,	10,		10 );
		zoom->AddHorizontalBand( 12,	12,		10 );
		zoom->AddHorizontalBand( 14,	14,		10 );
		zoom->AddHorizontalBand( 16,	16,		10 );
		zoom->AddHorizontalBand( 32,	32,		10 );

		zoom->SetUpdatedMessage(new BMessage('ibez') );
		zoom->StartWatching(this, 'ibez');
		zoom->SetZoomX(10);
		if (addTo) addTo->AddChild(zoom);
	}
	/* Add the colour control.
	 */
	BPoint			ctrlPt(frame.left, r.bottom + 21);
	BColorControl*	ctrl = new BColorControl(	ctrlPt,
												B_CELLS_32x8, 2, "color_control");

	if (ctrl) {
		ctrl->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		AddChild(ctrl);
	}
	/* Add the alpha control.
	 */
	ArpKnobPanel*	knob = new ArpKnobPanel("alpha knob", "Alpha", NULL, 0, 255, true, B_VERTICAL);
	if (knob) {
		knob->MoveTo(r.left + 272, ctrlPt.y - 2);
		knob->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		knob->KnobControl()->SetValue(255);
		AddChild(knob);
	}

	if (iconView && ctrl && knob) iconView->SetControls(ctrl, knob->KnobControl());
}


/*************************************************************************
 * ICON-DROP-VIEW
 *************************************************************************/
IconDropView::IconDropView(	BRect frame, BBitmap* bitmap,
							AmActiveToolView* activeTools)
		: inherited(frame, ICON_VIEW_STR, B_FOLLOW_ALL, B_WILL_DRAW),
		  mBitmap(bitmap), mActiveTools(activeTools), mColorCtrl(NULL),
		  mKnobCtrl(NULL), mPixelSize(10), mHasChanges(false),
		  mBitmapChangeMsg(NULL)
{
	//if (mBitmap) mPixelAccess.set_to( mBitmap->ColorSpace() );
	mBgC.red = mBgC.blue = mBgC.alpha = 255;
	mBgC.green = 0;
}

IconDropView::~IconDropView()
{
	delete mBitmapChangeMsg;
}

void IconDropView::SetControls(BColorControl* colorCtrl, ArpKnobControl* knobCtrl)
{
	mColorCtrl = colorCtrl;
	mKnobCtrl = knobCtrl;
}

void IconDropView::SetBitmap(BBitmap* bitmap)
{
	mBitmap = bitmap;
	//if (mBitmap) mPixelAccess.set_to( mBitmap->ColorSpace() );
	SetHasChanges(false);
	Invalidate();
}

void IconDropView::SetBitmapChangeMessage(BMessage* msg)
{
	mBitmapChangeMsg = msg;
}

void IconDropView::SetZoom(float zoom)
{
	mPixelSize = zoom;
	Invalidate();
}

void IconDropView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void IconDropView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void IconDropView::GetPreferredSize(float *width, float *height)
{
	*width = *height = 0;
	if (!mBitmap) return;
	BRect	 r = mBitmap->Bounds();
	*width = mPixelSize * (r.Width() + 1);
	*height = mPixelSize * (r.Height() + 1);
}

/* My own function for setting the bits -- I don't care
 * sources larger then my destination are truncated.
 */
/*static void set_bits(BBitmap* src, BBitmap* dest, pixel_access& pixelAccess)
{
	BRect		srcRect = src->Bounds(), destRect = dest->Bounds();
	float		width = (srcRect.Width() >= destRect.Width() )
							? destRect.Width() : srcRect.Width();
	float		height = (srcRect.Height() >= destRect.Height() )
							? destRect.Height() : srcRect.Height();
	for (float y = 0; y <= height; y++) {
		for (float x = 0; x <= width; x++) {
			rgb_color		c = read_pixel(src, x, y, pixelAccess);
			write_pixel(dest, x, y, c, pixelAccess);
		}
	}
}*/

void IconDropView::MessageReceived(BMessage* msg)
{
	if (msg->WasDropped() ) HandleRefDrop(msg);

	inherited::MessageReceived(msg);
}

void IconDropView::MouseDown(BPoint where)
{
	uint32		activeToolCode = 0;
	if (mActiveTools) {
		int32		button = 0;
		Window()->CurrentMessage()->FindInt32("buttons", &button);
		activeToolCode = mActiveTools->ActiveToolCode(button);
	}
	if (activeToolCode == PENCIL_TOOL_CODE) WritePixel(where);
	else if (activeToolCode == DROPPER_TOOL_CODE) ReadPixel(where);
}

void IconDropView::MouseMoved(	BPoint where,
								uint32 code,
								const BMessage *a_message)
{
	inherited::MouseMoved(where, code, a_message);
	if (a_message) return;

	uint32		activeToolCode = 0;
	if (mActiveTools) {
		int32		button = 0;
		Window()->CurrentMessage()->FindInt32("buttons", &button);
		activeToolCode = mActiveTools->ActiveToolCode(button);
	}
	if (activeToolCode == PENCIL_TOOL_CODE) WritePixel(where);
}

status_t IconDropView::Copy()
{
	if (!mBitmap) return B_ERROR;
	BMessage	bmMsg;
	if (mBitmap->Archive(&bmMsg) != B_OK) return B_ERROR;
	if (!be_clipboard->Lock() ) return B_ERROR;
	be_clipboard->Clear();
	BMessage*	clip = be_clipboard->Data();
	if (clip) {
		clip->AddMessage("image/x-vnd.Be-bitmap", &bmMsg);
		clip->AddRect("rect", mBitmap->Bounds());
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();
	return B_OK;
}

status_t IconDropView::Paste()
{
	if (!mBitmap || !be_clipboard->Lock() ) return B_ERROR;
	BMessage*	clip = be_clipboard->Data();
	if (!clip) {
		be_clipboard->Unlock();
		return B_ERROR;
	}

	BMessage	bmMsg;
	if (clip->FindMessage("image/x-vnd.Be-bitmap", &bmMsg) == B_OK) {
		BBitmap*	bm = dynamic_cast<BBitmap*>( BBitmap::Instantiate(&bmMsg) );
		if (bm) {
			//set_bits(bm, mBitmap, mPixelAccess);
			SetHasChanges(true);
			Invalidate();
			delete bm;
		}
	}
	be_clipboard->Unlock();
	return B_OK;
}

status_t IconDropView::FlipVertically()
{
	if (!mBitmap) return B_ERROR;
	BBitmap*		src = new BBitmap(mBitmap);
	if (!src) return B_NO_MEMORY;
	BRect			b(src->Bounds() );
	if (b != mBitmap->Bounds() ) {
		delete src;
		return  B_ERROR;
	}
	for (int32 y = 0; y <= int32(b.bottom); y++) {
		for (int32 x = 0; x <= int32(b.right); x++) {
			//rgb_color		c = read_pixel(src, x, y, mPixelAccess);
			//write_pixel(mBitmap, x, b.bottom - y, c, mPixelAccess);
		}
	}

	delete src;
	SetHasChanges(true);
	Invalidate();
	return B_OK;
}

status_t IconDropView::FlipHorizontally()
{
	if (!mBitmap) return B_ERROR;
	BBitmap*		src = new BBitmap(mBitmap);
	if (!src) return B_NO_MEMORY;
	BRect			b(src->Bounds() );
	if (b != mBitmap->Bounds() ) {
		delete src;
		return  B_ERROR;
	}
	for (int32 x = 0; x <= int32(b.right); x++) {
		for (int32 y = 0; y <= int32(b.bottom); y++) {
			//rgb_color		c = read_pixel(src, x, y, mPixelAccess);
			//write_pixel(mBitmap, b.right - x, y, c, mPixelAccess);
		}
	}

	delete src;
	SetHasChanges(true);
	Invalidate();
	return B_OK;
}

status_t IconDropView::FillAlpha()
{
	if (!mBitmap) return B_ERROR;
	uint8		alpha = CurrentColor().alpha;
	BRect		b(mBitmap->Bounds() );
	/*for (int32 x = 0; x <= int32(b.right); x++) {
		for (int32 y = 0; y <= int32(b.bottom); y++) {
			rgb_color		c = read_pixel(mBitmap, x, y, mPixelAccess);
			if (c.alpha > alpha) {
				c.alpha = alpha;
				write_pixel(mBitmap, x, y, c, mPixelAccess);
			}
		}
	}*/
	SetHasChanges(true);
	Invalidate();
	return B_OK;
}

void IconDropView::DrawOn(BRect clip, BView* view)
{
	view->SetHighColor( tint_color(Prefs().Color(AM_AUX_WINDOW_BG_C), B_DARKEN_1_TINT) );
	view->FillRect(clip);
	if (!mBitmap) return;
	BRect					b = mBitmap->Bounds();
	rgb_color				c;
	drawing_mode			mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	ArpLineArrayCache		mLines(view);
	mLines.BeginLineArray();
	for (float y = 0; y <= b.bottom; y++) {
		for (float x = 0; x <= b.right; x++) {
			BRect			r(x*mPixelSize, y*mPixelSize, ((x+1)*mPixelSize)-1, ((y+1)*mPixelSize)-1);
			if (r.Intersects(clip) ) {
				//c = read_pixel(mBitmap, x, y, mPixelAccess);

				view->SetHighColor(mBgC);
				view->FillRect(r);
				//view->SetHighColor(c);
				/* Diverge the drawing path -- if the bitmap is so small that I
				 * can't realistically do my fancy shading, then don't worry about it.
				 */
				if (mPixelSize < 4) {
					//view->FillRect(r);
				} else {
					//view->FillRect( BRect(r.left + 1, r.top + 1, r.right - 2, r.bottom - 2) );

					//rgb_color	c2 = change_color(c, 150);
					//mLines.AddLine(BPoint(r.left, r.top), BPoint(r.right - 1, r.top), c2);
					//mLines.AddLine(BPoint(r.left, r.top + 1), BPoint(r.left, r.bottom - 1), c2);

					//c2 = change_color(c, -150);
					//mLines.AddLine(BPoint(r.left + 1, r.bottom - 1), BPoint(r.right - 1, r.bottom - 1), c2);
					//mLines.AddLine(BPoint(r.right - 1, r.top + 1), BPoint(r.right - 1, r.bottom - 2), c2);

					//mLines.AddLine(BPoint(r.right, r.top), BPoint(r.right, r.bottom), 0, 0, 0);
					//mLines.AddLine(BPoint(r.left, r.bottom), BPoint(r.right - 1, r.bottom), 0, 0, 0);
				}
			}
		}
	}
	mLines.EndLineArray();
	view->SetDrawingMode(mode);
}
	
void IconDropView::HandleRefDrop(BMessage* msg)
{
	if (!mBitmap) return;
	entry_ref		ref;
	if (msg->FindRef("refs", &ref) != B_OK) return;
	BBitmap*		bm = BTranslationUtils::GetBitmap(&ref);
	if (bm) {
		//set_bits(bm, mBitmap, mPixelAccess);
		SetHasChanges(true);
		Invalidate();
		delete bm;
	}
}

void IconDropView::WritePixel(BPoint where)
{
	if (!mColorCtrl || !mKnobCtrl || !mBitmap) return;
	BPoint			pixel = PixelAt(where);
	if ( !(mBitmap->Bounds().Contains(pixel)) ) return;
	//rgb_color		oldC = read_pixel(mBitmap, pixel.x, pixel.y, mPixelAccess);
	rgb_color		newC = CurrentColor();
	//if (colors_equal(oldC, newC) ) return;
	//write_pixel(mBitmap, pixel.x, pixel.y, newC, mPixelAccess);
	SetHasChanges(true);
	Invalidate(BRect(	pixel.x * mPixelSize, pixel.y * mPixelSize,
						((pixel.x + 1) * mPixelSize) - 1, ((pixel.y + 1) * mPixelSize) -1) );
}

void IconDropView::ReadPixel(BPoint where)
{
	if (!mColorCtrl || !mKnobCtrl || !mBitmap) return;
	BPoint			pixel = PixelAt(where);
	if ( !(mBitmap->Bounds().Contains(pixel)) ) return;
	//rgb_color		c = read_pixel(mBitmap, pixel.x, pixel.y, mPixelAccess);
	//mColorCtrl->SetValue(c);
	//mKnobCtrl->SetValue(c.alpha);
}

BPoint IconDropView::PixelAt(BPoint where) const
{
	return BPoint(	floor(where.x / mPixelSize),
					floor(where.y / mPixelSize) );
}

rgb_color IconDropView::CurrentColor()
{
	ArpASSERT(mColorCtrl && mKnobCtrl);
	rgb_color		c = mColorCtrl->ValueAsColor();
	c.alpha = (uint8)mKnobCtrl->Value();
	return c;
}

void IconDropView::SetHasChanges(bool hasChanges)
{
	if (mHasChanges != hasChanges && mBitmapChangeMsg && Window() ) {
		BMessage		msg(*mBitmapChangeMsg);
		msg.AddBool("bitmap changes", hasChanges);
		Window()->PostMessage(&msg);
	}
	mHasChanges = hasChanges;
}

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
/*static rgb_color read_pixel(const BBitmap* bm, float x, float y, pixel_access& pa)
{
	uint8*		pixel = (uint8*)( ((uint8*)bm->Bits()) + (uint32)(x * pa.bpp() ) + (uint32)(y * bm->BytesPerRow() ) );
	return pa.read(pixel);
}*/

/*static void write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa)
{
	uint8*		pixel = (uint8*)( ((uint8*)bm->Bits()) + (uint32)(x * pa.bpp() ) + (uint32)(y * bm->BytesPerRow() ) );
	pa.write(pixel, c);
}*/

static rgb_color change_color(rgb_color c1, int32 change)
{
	rgb_color	c2;
	c2.alpha = c1.alpha;
	if (change > 0) {
		if ( (int32)c1.red + change >= 255 ) c2.red = 255; else c2.red = c1.red + change;
		if ( (int32)c1.green + change >= 255 ) c2.green = 255; else c2.green = c1.green + change;
		if ( (int32)c1.blue + change >= 255 ) c2.blue = 255; else c2.blue = c1.blue + change;
	} else {
		if ( (int32)c1.red + change <= 0 ) c2.red = 0; else c2.red = c1.red + change;
		if ( (int32)c1.green + change <= 0 ) c2.green = 0; else c2.green = c1.green + change;
		if ( (int32)c1.blue + change <= 0 ) c2.blue = 0; else c2.blue = c1.blue + change;
	}
	return c2;
}

static inline bool colors_equal(rgb_color c1, rgb_color c2)
{
	return c1.red == c2.red && c1.green == c2.green
			&& c1.blue == c2.blue && c1.alpha == c2.alpha;
}

void seq_write_pixel(const BBitmap* bm, float x, float y, rgb_color c)
{
	//pixel_access	pa( bm->ColorSpace() );
	//uint8*		pixel = (uint8*)( ((uint8*)bm->Bits()) + (uint32)(x * pa.bpp() ) + (uint32)(y * bm->BytesPerRow() ) );
	//pa.write(pixel, c);
}

