/*
	
	ArpRootLayout.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	The top of a layout tree.
*/

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#include "ArpLayout/ArpRootLayout.h"
#endif

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#include <ArpLayout/ArpGlobalSet.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include "ArpKernel/ArpColor.h"
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _MENU_H
#include <interface/Menu.h>
#endif

#include <support/Autolock.h>
#include <float.h>

ArpMOD();

static const char* GlobalsParam = "ArpRootLayoutable:Globals";

/* ------ ArpRootLayout constructor and destructor ------
 *
 * The various ways to create and destroy ArpRootLayout
 * objects.
 */
 
ArpRootLayout::ArpRootLayout(BRect frame, const char* name,
							 uint32 resizeMask, uint32 flags)
	: ArpLayoutView(frame,name,resizeMask,flags)
{
	ArpD(cdb << ADH << "Creating root layout " << name << endl);
	
	ArpColor stdBack(ui_color(B_PANEL_BACKGROUND_COLOR));
	ArpColor stdText(0x00, 0x00, 0x00);
	ArpColor fillBack(0xff, 0xff, 0xff);
	ArpColor fillText(0x00, 0x40, 0x40);
	
	mGlobals.AddData("StdBackColor",B_RGB_COLOR_TYPE,stdBack,sizeof(rgb_color));
	mGlobals.AddData("StdTextColor",B_RGB_COLOR_TYPE,stdText,sizeof(rgb_color));
	mGlobals.AddData("FillBackColor",B_RGB_COLOR_TYPE,fillBack,sizeof(rgb_color));
	mGlobals.AddData("FillTextColor",B_RGB_COLOR_TYPE,fillText,sizeof(rgb_color));
	
	MakeDefaultGlobals(mGlobals);
	
	ArpD(cdb << ADH << "Placing root globals into layout..." << endl);
	SetGlobals(this);
	
	ArpD(cdb << ADH << "Setting layout to current bounds..." << endl);
	ArpBaseLayout::SetLayout(Bounds());
}

ArpRootLayout::ArpRootLayout(BMessage* data, bool final)
	: ArpLayoutView(data, false)
{
	if( data ) {
		data->FindMessage(GlobalsParam, &mGlobals);
	}
	
	initialize();
	
	MakeDefaultGlobals(mGlobals);
	
	if( final ) InstantiateParams(data);
	
	SetGlobals(this);
	ArpBaseLayout::SetLayout(Bounds());
}

void ArpRootLayout::initialize()
{
}

ArpRootLayout::~ArpRootLayout()
{
	SetLayoutInhibit(true);
	SetGlobals(NULL);
}

/* ------------ ArpRootLayout archiving ------------
 *
 * Archiving and retrieving ArpRootLayout objects.
 */
BArchivable* ArpRootLayout::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpRootLayout") ) 
		return new ArpRootLayout(archive); 
	return NULL;
}

status_t ArpRootLayout::Archive(BMessage* data, bool deep) const
{
	status_t status = inherited::Archive(data, deep);
	if( status == B_NO_ERROR )
		status = data->AddMessage(GlobalsParam, &mGlobals);
	return status;
}

void ArpRootLayout::AttachedToWindow()
{
	ArpD(cdb << ADH << "Root " << Name()
				 << ": this view is attached to window "
				 << Window()->Title() << endl);
	
	inherited::AttachedToWindow();
}

void ArpRootLayout::AllAttached()
{
	ArpD(cdb << ADH << "Root " << Name()
				 << ": all views are now attached to window "
				 << Window()->Title() << endl);
	
	inherited::AllAttached();
	
	// When an ArpLayout class is first created, it starts
	// with its LayoutInhibit flag set: this tells it not
	// to do relayouts in response to changes in its
	// children etc., until the flag is turned off.  There
	// are two reasons for doing this.  The most obvious is
	// that it saves a lot of overhead when constructing the
	// initial object hierarchy, as it's not recomputing the
	// layout every time an object is added.  However, there
	// is an even more important reason: many Be objects
	// don't supply sane GetPreferredSize() dimensions until
	// they are added to a window.  By waiting until now
	// to allow the layout to happen, we don't have to worry
	// about cacheing any incorrect dimension information
	// picked up from before the UI was added to the window.
	SetLayoutInhibit(false);
	
	// If this is the top-level view in the window, set up
	// the size limits of the window from the layout.
	if( Parent() == 0 ) SetWindowLimits();
}

void ArpRootLayout::DetachedFromWindow()
{
	ArpD(cdb << ADH << "Root " << Name()
				 << ": all views are now detached from window." << endl);
	
	// Similarily, disable updates when removed from a window.
	SetLayoutInhibit(true);
	
	inherited::DetachedFromWindow();
}

void ArpRootLayout::FrameResized(float width, float height)
{
	if( Window() ) {
		ArpD(cdb << ADH <<"-------------------------------------------" << endl);
		ArpD(cdb << ADH << "ArpRootLayout::FrameResized()" << endl);
		ArpD(cdb << ADH << "  width=" << width << ", height=" << height << endl);
		BRect bounds = Bounds();
		ArpD(cdb << ADH << "  Resizing to bounds: " << bounds << endl);
		Window()->BeginViewTransaction();
		ArpBaseLayout::SetLayout(bounds);
		Window()->EndViewTransaction();
	}
}

void ArpRootLayout::WindowActivated(bool state)
{
	BView::WindowActivated(state);
	inherited::SetLayoutActivated(state);
}

void ArpRootLayout::MessageReceived(BMessage *message)
{
	if( message ) {
		switch(message->what) {
			case ARP_GLOBALS_MSG: {
				UpdateGlobals(message);
			} break;
			default: {
				BView::MessageReceived(message);
			} break;
		}
	}
}

void ArpRootLayout::Draw(BRect updateRect)
{
	ArpD(cdb << ADH << "Root " << Name()
				 << ": drawing window with frame "
				 << updateRect
				 << " (bounds=" << Bounds() << ")" << endl);
	inherited::Draw(updateRect);
}

void ArpRootLayout::FrameMoved(BPoint new_position)
{
	ArpD(cdb << ADH << "Root " << Name()
				 << ": frame moved to "
				 << new_position << endl);
	inherited::FrameMoved(new_position);
}

void ArpRootLayout::SetWindowLimits()
{
	BWindow* win = Window();
	if( win && strcmp(win->Name(), "bm") ) {
		ArpD(cdb << ADH << "Window Name=" << win->Name()
				<< ", Sem=" << win->Sem()
				<< ", Locked=" << win->IsLocked()
				<< ", Locks=" << win->CountLocks()
				<< ", LockRequests=" << win->CountLockRequests()
				<< endl);
		float minw=0,minh=0,maxw=0,maxh=0;
		const ArpDimens& dimens = LayoutDimens();
		win->GetSizeLimits(&minw,&maxw,&minh,&maxh);
		win->SetSizeLimits(dimens.X().TotalMin(), maxw,
						   dimens.Y().TotalMin(), maxh);
		win->SetZoomLimits(dimens.X().TotalPref(), dimens.Y().TotalPref());
		BRect bounds = win->Bounds();
		ArpD(cdb << ADH << "SetWindowLimits():" << endl);
		ArpD(cdb << ADH << "  Bounds: " << bounds << ", minw=" << minw
						<< ", minh=" << minh << ", maxw=" << maxw
						<< ", maxh=" << maxh << endl);
		ArpD(cdb << ADH << "  Layout minw=" << dimens.X().TotalMin()
						<< ", minh=" << dimens.Y().TotalMin() << endl);
		float bw = bounds.Width();
		float bh = bounds.Height();
		if( bw < dimens.X().TotalMin() || bh < dimens.Y().TotalMin() ) {
			if( bw < dimens.X().TotalMin() ) bw = dimens.X().TotalMin();
			if( bh < dimens.Y().TotalMin() ) bh = dimens.Y().TotalMin();
			win->ResizeTo(bw, bh);
		}
	}
}

status_t ArpRootLayout::AddGlobals(const BMessage* values)
{
	status_t res = B_NO_ERROR;
	if( values ) {
		ArpD(cdb << ADH << "Adding global values: " << *values << endl);
		char* name;
		ulong type;
		long count;
		for( int32 i=0; !values->GetInfo(B_ANY_TYPE,i,&name,&type,&count); i++ ) {
			ArpD(cdb << ADH << "Adding global " << name
							<< " (type=" << type <<")." << endl);
			if( !mGlobals.HasData(name,type) ) {
				const void* data;
				ssize_t size;
				if( !(res=values->FindData(name,type,&data,&size)) ) {
					res = mGlobals.AddData(name,type,data,size);
				} else {
					ArpD(cdb << ADH << "Ehhh?  Error getting global: res="
								<< res << "." << endl);
				}
			} else {
				type = 0;
				int32 count;
				mGlobals.GetInfo(name,&type,&count);
				ArpD(cdb << ADH << "Whoops, already have it!  Type is "
							<< type << "." << endl);
			}
		}
	}
	
	return res;
}

status_t ArpRootLayout::UpdateGlobals(const BMessage* newVals)
{
	bool oldInhibit = LayoutInhibit();
	SetLayoutInhibit(true);
	
	arp_update_message(mGlobals, *newVals);
	if( Window() ) Window()->BeginViewTransaction();
	
	ArpD(cdb << ADH << "Global update msg = " << *newVals
				<< endl);
	ArpD(cdb << ADH << "Final global values = " << mGlobals
				<< endl);
	ArpD(cdb << ADH << "-------------------------------------" << endl);
	ArpD(cdb << ADH << "Refreshing globals:" << endl);
	ArpGlobalUpdate update(newVals);
	SetGlobals(&update);
	
	SetLayoutInhibit(oldInhibit);
	
	if( Window() && strcmp(Window()->Name(), "bm") ) {
		ArpD(cdb << ADH << "-------------------------------------" << endl);
		ArpD(cdb << ADH << "Getting dimensions:" << endl);
		BRect bounds = Bounds();
		const ArpDimens& dimens = LayoutDimens();
		if( Parent() == 0 ) {		// top-level view in window?
			if( bounds.Width() < dimens.X().TotalMin() ||
				bounds.Height() < dimens.Y().TotalMin() ) {
				ArpD(cdb << ADH << "-------------------------------------"
						<< endl);
				ArpD(cdb << ADH << "Resizing window:" << endl);
				Window()->ResizeTo(dimens.X().TotalMin(),dimens.Y().TotalMin());
				bounds = Window()->Bounds();
			}
			SetWindowLimits();
		}
		ArpD(cdb << ADH << "-------------------------------------" << endl);
		ArpD(cdb << ADH << "Setting layout:" << endl);
		ArpBaseLayout::SetLayout(bounds);
	}
	
	ArpD(cdb << ADH << "-------------------------------------" << endl);
	ArpD(cdb << ADH << "Enabling updates:" << endl);
	if( Window() ) Window()->EndViewTransaction();
	ArpD(cdb << ADH << "-------------------------------------" << endl);
	ArpD(cdb << ADH << "Done!" << endl);
	
	return B_OK;
}

void ArpRootLayout::MakeDefaultGlobals(BMessage& globals)
{
	globals.RemoveName("PlainFont");
	globals.RemoveName("BoldFont");
	globals.RemoveName("FixedFont");
	globals.RemoveName("MenuBackColor");
	globals.RemoveName("MenuTextColor");
	globals.RemoveName("MenuFont");
	
	AddMessageFont(&globals, "PlainFont", be_plain_font);
	AddMessageFont(&globals, "BoldFont", be_bold_font);
	AddMessageFont(&globals, "FixedFont", be_fixed_font);
	
	// Standard menu style
	BFont menuFont(be_bold_font);
	ArpColor menuBack(ui_color(B_PANEL_BACKGROUND_COLOR));
	ArpColor menuText(0x00,0x00,0x00);
	struct menu_info minfo;
	if( get_menu_info(&minfo) == B_NO_ERROR ) {
		menuFont.SetFamilyAndStyle(minfo.f_family,minfo.f_style);
		menuFont.SetSize(minfo.font_size);
		menuBack = minfo.background_color;
	}
	globals.AddData("MenuBackColor",B_RGB_COLOR_TYPE,menuBack,sizeof(rgb_color));
	globals.AddData("MenuTextColor",B_RGB_COLOR_TYPE,menuText,sizeof(rgb_color));
	AddMessageFont(&globals, "MenuFont", &menuFont);
}
