#include <stdio.h>
#include <be/interface/Bitmap.h>
#include <be/interface/MenuItem.h>
#include <be/interface/PopUpMenu.h>
#include <be/interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpBackground.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewFactory.h"

static const BBitmap*	prop_on_image = 0;
static const BBitmap*	prop_off_image = 0;
static const BBitmap*	slice_info_top_bg = 0;

/*************************************************************************
 * _AM-INFO-ADORNMENT
 * An abstract superclass for views that appear as adornment in the
 * title view.
 *************************************************************************/
class _AmInfoAdornment : public BView
{
public:
	_AmInfoAdornment(	BRect frame,
						const char *name,
						uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW); 
	virtual ~_AmInfoAdornment();

	virtual	void AttachedToWindow();

protected:
	virtual void DrawExtrudedBackground(BRect r);
	virtual void DrawDepressedBackground(BRect r);
	
private:
	typedef BView	inherited;
};

/*************************************************************************
 * _AM-PROPERTIES-FIELD
 * A simple class that invokes a menu.  This can be used as adornment in
 * the title view.
 *************************************************************************/
class _AmPropertiesField : public _AmInfoAdornment
{
public:
	_AmPropertiesField(	BPoint topLeft,
						BPopUpMenu* menu,
						AmTrackInfoView* menuTarget);
	virtual ~_AmPropertiesField();

	virtual	void AttachedToWindow();
	virtual	void MouseDown(BPoint where);
	virtual void Draw(BRect clip);

private:
	typedef _AmInfoAdornment	inherited;
	BPopUpMenu*					mMenu;
	/* The view that will receive the messages delivered by mMenu.
	 */
	AmTrackInfoView*			mMenuTarget;
};

/*************************************************************************
 * AM-TRACK-INFO-VIEW
 * This simple class displays horizontal bars (like sheet music).
 *************************************************************************/
AmTrackInfoView::AmTrackInfoView(	BRect frame,
									const char* name,
									AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									TrackViewType viewType)
		: inherited(frame, name, B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		  mSongRef(songRef), mTrackWinProps(trackWinProps), mViewType(viewType),
		  mHeadBackground(NULL), highlight(false)
{
	if (!slice_info_top_bg) slice_info_top_bg = ImageManager().FindBitmap( SLICE_INFO_TOP_BG );
}

AmTrackInfoView::~AmTrackInfoView()
{
	delete mHeadBackground;
}

void AmTrackInfoView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	mViewColor = Prefs().Color(AM_INFO_BG_C);
	SetViewColor(B_TRANSPARENT_COLOR);
	AddPropertiesField();
}

void AmTrackInfoView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
}

void AmTrackInfoView::Highlight() {
	if (highlight == true) return;
	
	highlight = true;
	Invalidate();
}

void AmTrackInfoView::Unhighlight() {
	if (highlight == false) return;
	
	highlight = false;
	Invalidate();
}

void AmTrackInfoView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void AmTrackInfoView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case AM_ORDERED_TRACK_MSG:
			break;
		case S_DRAGGED_TRACK_DATA:
			HandleDraggedTrackData(msg);
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmTrackInfoView::ControlActivated(int32 code)
{
}

void AmTrackInfoView::AddBackground(ArpBackground* background)
{
	if (mHeadBackground) mHeadBackground->AddTail(background);
	else mHeadBackground = background;
}

void AmTrackInfoView::DrawOn(BRect clip, BView* view)
{
	if (highlight) view->SetHighColor(180,180,180);
	else view->SetHighColor(mViewColor);
	view->FillRect( BRect(clip) );
	
	if (mHeadBackground) mHeadBackground->DrawAllOn(view, clip);

	PreDrawSliceOn(view, clip);
	if (slice_info_top_bg) {
		drawing_mode	mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->DrawBitmapAsync( slice_info_top_bg, BPoint(0, 0) );
		view->SetDrawingMode(mode);
	}
	PostDrawSliceOn(view, clip);
}

static rgb_color mix_wont_work(rgb_color c1, rgb_color c2, float amount)
{
	rgb_color	c;
	c.red = c1.red - (uint8)( (c1.red - c2.red) * amount);	
	c.green = c1.green - (uint8)( (c1.green - c2.green) * amount);	
	c.blue = c1.blue - (uint8)( (c1.blue - c2.blue) * amount);	
	c.alpha = 255;
	return c;
}

void AmTrackInfoView::PreDrawSliceOn(BView* view, BRect clip)
{
	BRect		b = Bounds();
	rgb_color	black;
	black.red = black.green = black.blue = 0;
	black.alpha = 255;
	if (clip.right >= b.right - 3) {
		view->SetHighColor( mix_wont_work(mViewColor, black, 0.15) );
		view->StrokeLine( BPoint(b.right - 3, clip.top), BPoint(b.right - 3, clip.bottom) );
	}
	if (clip.right >= b.right - 2) {
		view->SetHighColor( mix_wont_work(mViewColor, black, 0.30) );
		view->StrokeLine( BPoint(b.right - 2, clip.top), BPoint(b.right - 2, clip.bottom) );
	}
	if (clip.right >= b.right - 1) {
		view->SetHighColor( mix_wont_work(mViewColor, black, 0.55) );
		view->StrokeLine( BPoint(b.right - 1, clip.top), BPoint(b.right - 1, clip.bottom) );
	}
}

void AmTrackInfoView::PostDrawSliceOn(BView* view, BRect clip)
{
	BRect	b = Bounds();
	if (clip.right >= b.right) {
		view->SetHighColor(0, 0, 0);
		view->StrokeLine( BPoint(b.right, clip.top), BPoint(b.right, clip.bottom) );
	}
}

void AmTrackInfoView::HandleDraggedTrackData(BMessage *msg) {
#if 0
	BWindow		*win;
	long		dragIndex /*, index = Index()*/;
	if ( (msg->FindPointer(STR_WINDOW, (void**)&win) != B_OK)
			|| (msg->FindInt32(STR_POSITION, &dragIndex) != B_OK) )
		return;
			
	// Dragging across windows is not currently supported.
	if (win != Window()) return;

	// if we are next to each other, exchange our positions
//	if ( ((dragIndex - index) == 1) || ((index - dragIndex) == 1) ) {
// FINISH		
#endif
}

BPopUpMenu*	AmTrackInfoView::NewPropertiesMenu() const
{
	BPopUpMenu*				menu = new BPopUpMenu( "properties menu" );
	if( !menu ) return 0;
	BMenuItem*				item;
	/* The Duplicate menu item.
	 */
	BMessage*				msg = new BMessage( DUPLICATE_INFO_MSG );
	if( msg && (item = new BMenuItem( "Duplicate", msg )) ) {
		msg->AddPointer( SZ_VIEW_ID, this );
		msg->AddString( SZ_FACTORY_SIGNATURE, mFactorySignature.String() );
		msg->AddString( SZ_FACTORY_VIEW_NAME, mViewName.String() );
		menu->AddItem( item );
	}
	/* The Change View menu item.
	 */
	if( (item = NewChangeViewItem()) ) menu->AddItem( item );
	/* The Remove menu item.
	 */
	msg = new BMessage( REMOVE_INFO_MSG );
	if( msg && (item = new BMenuItem( "Remove", msg )) ) {
		msg->AddPointer( SZ_VIEW_ID, this );
		menu->AddItem( item );
	}
	return menu;
}

BMenuItem* AmTrackInfoView::NewChangeViewItem() const
{
	AmViewFactory*	factory = AmGlobals().FactoryNamed(mFactorySignature);
	if (!factory) return NULL;
	BMenu*			menu = new BMenu(AM_INFO_CHANGE_VIEW_STR);
	if (!menu) return NULL;
	BMenuItem*		item;		
	BString			name;
	for (uint32 k = 0; factory->DataNameAt(k, mViewType, name) == B_OK; k++) {
		BMessage*	msg = new BMessage(CHANGE_INFO_MSG);
		if( msg && (item = new BMenuItem( name.String(), msg )) ) {
			msg->AddPointer(SZ_VIEW_ID, this);
			msg->AddString(SZ_FACTORY_SIGNATURE, mFactorySignature.String() );
			msg->AddString(SZ_FACTORY_VIEW_NAME, name.String() );
			item->SetTarget(this);
			menu->AddItem(item);
			if (name == mViewName) item->SetEnabled(false);
		}
		name = (const char*)NULL;
	}
	item = new BMenuItem(menu);
	if( !item ) {
		delete menu;
		return NULL;
	}
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont( be_plain_font );
	return item;
}

void AmTrackInfoView::AddPropertiesField()
{
	BPopUpMenu*				menu = NewPropertiesMenu();
	if (!menu) return;
	_AmPropertiesField*		field = new _AmPropertiesField( BPoint(0, 0),
															menu,
															this );
	if (field) AddChild(field);
}

// #pragma mark -

/*************************************************************************
 * _AM-INFO-ADORNMENT
 *************************************************************************/
_AmInfoAdornment::_AmInfoAdornment(	BRect frame,
									const char* name,
									uint32 resizeMask,
									uint32 flags )
		: BView(frame, name, resizeMask, flags)
{
	if( !prop_on_image ) prop_on_image = ImageManager().FindBitmap( SLICE_PROPERTY_MENU_NORMAL_IMAGE_STR );
	if( !prop_off_image ) prop_off_image = ImageManager().FindBitmap( SLICE_PROPERTY_MENU_PRESSED_IMAGE_STR );
}

_AmInfoAdornment::~_AmInfoAdornment()
{
}

void _AmInfoAdornment::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor( B_TRANSPARENT_COLOR );
}

void _AmInfoAdornment::DrawExtrudedBackground(BRect r)
{
	if( prop_on_image )
		DrawBitmapAsync( prop_on_image, BPoint(0, 0) );
}

void _AmInfoAdornment::DrawDepressedBackground(BRect r)
{
	if( prop_off_image )
		DrawBitmapAsync( prop_off_image, BPoint(0, 0) );
}

// #pragma mark -

/*************************************************************************
 * _AM-PROPERTIES-FIELD
 *************************************************************************/
_AmPropertiesField::_AmPropertiesField(	BPoint topLeft,
										BPopUpMenu* menu,
										AmTrackInfoView* menuTarget)
		: inherited( BRect(	topLeft.x,
							topLeft.y,
							topLeft.x + Prefs().Size( PROP_FIELD_X ),
							topLeft.y + Prefs().Size( PROP_FIELD_Y) ),
					 "propertiesfield"),
		  mMenu(menu), mMenuTarget(menuTarget)
{
	assert(menu);
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont( be_plain_font );
}

_AmPropertiesField::~_AmPropertiesField()
{
	delete mMenu;
}

void _AmPropertiesField::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mMenu && mMenuTarget) mMenu->SetTargetForItems(mMenuTarget);
}

void _AmPropertiesField::MouseDown(BPoint where)
{
	if (mMenu) {
		BRect	b = Bounds();
		DrawDepressedBackground(b);
		BPoint	pt(b.left, b.bottom);
		BRect	r(pt, pt);
		if (mMenuTarget) mMenuTarget->ControlActivated(0);
		mMenu->Go(ConvertToScreen(pt), true, false, ConvertToScreen(r));
		Invalidate();
	}
}

void _AmPropertiesField::Draw(BRect clip)
{
	inherited::Draw( clip );
	BRect		b = Bounds();
	DrawExtrudedBackground(b);
}
