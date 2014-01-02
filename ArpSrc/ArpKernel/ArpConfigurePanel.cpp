/*
	
	ArpConfigurePanel.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#ifndef ARPKERNEL_ARPCONFIGUREPANEL_H
#include <ArpKernel/ArpConfigurePanel.h>
#endif

#ifndef ARPCOLLECTIONS_ARPPTRVECTOR_H
#include <ArpCollections/ArpPtrVector.h>
#endif

#ifndef ARPCOLLECTIONS_ARPSTLVECTOR_H
#include <ArpCollections/ArpSTLVector.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _BUTTON_H
#include <interface/Button.h>
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _PATH_H
#include <storage/Path.h>
#endif

#include <stdlib.h>

ArpMOD();

ArpConfigurePanel::ArpConfigurePanel(BRect frame, const char* name,
								const ArpConfigureFile& config,
								button_width width,
								uint32 resizingMode,
								uint32 flags)
	: BTabView(frame, name, width, resizingMode, flags),
	  mConfig(config), mConfigViews(0), mInitSettings(0),
	  mMaxWidth(0), mMaxHeight(0), mTabWidth(0), mTabHeight(0)
{
	RebuildPanes();
}

ArpConfigurePanel::~ArpConfigurePanel()
{
	FreeMemory();
}

void ArpConfigurePanel::FreeMemory(void)
{
	delete mInitSettings;
	mInitSettings = 0;
	delete mConfigViews;
	mConfigViews = 0;
}

void ArpConfigurePanel::ClearPanes(void)
{
	BTab* tab = 0;
	while( (tab=RemoveTab(0)) != 0 ) {
		delete tab;
	}
	
	if( mInitSettings ) {
		for( size_t i=0; i<mInitSettings->size(); i++ ) {
			delete mInitSettings->at(i);
		}
	}
	
	FreeMemory();
}

void ArpConfigurePanel::RebuildPanes()
{
	//const bool isHidden = IsHidden();
	//Hide();
	
	BView* parent = Parent();
	if( parent != 0 ) RemoveSelf();
	
	ClearPanes();
	
	const ArpVectorI<ArpConfigurableI*>& configs = mConfig.Configs();
	ArpPtrVector<BView*> getViews;
	
	const BFont* font = be_plain_font;
	
	font_height fh;
	font->GetHeight(&fh);
	const float spaceh = ceil((fh.ascent+fh.descent+fh.leading)/2);
	const float spacew = ceil(font->StringWidth("W")/2);
	
	try {
		size_t i;
		
		ArpD(cdb << ADH << "Allocating vectors for " << configs.size()
					<< " configurable objects." << endl);
		
		// Initialize lists of configurable objects.
		mInitSettings = new ArpPtrVector<BMessage*>;
		mInitSettings->resize(configs.size(), 0);
		for( i=0; i<configs.size(); i++ ) {
			mInitSettings->at(i) = new BMessage;
		}
		
		// Retrieve all configuration views.
		mConfigViews = new ArpPtrVector<BView*>;
		for( i=0; i<configs.size(); i++ ) {
			configs[i]->GetConfiguration(mInitSettings->at(i));
			getViews.resize(0);
			ArpD(cdb << ADH << "Getting views for config #" << i
							<< " (" << configs[i] << ")" << endl);
			if( configs[i] && configs[i]->Configure(getViews) == B_OK ) {
				ArpD(cdb << ADH << "Found " << getViews.size() << endl);
				for( size_t j=0; j<getViews.size(); j++ ) {
					if( getViews[j] != 0 ) {
						mConfigViews->push_back(getViews[j]);
						getViews[j] = 0;
					}
				}
			}
		}
		
		// Set up dimensions of the tab view.
		BRect tabBound(Bounds());
		BRect tabFrame(Frame());
		mTabWidth = ceil(spacew*4 + (tabBound.Width()-tabFrame.Width()));
		mTabHeight = ceil(spaceh*4 + (tabBound.Height()-tabFrame.Height())
							+ TabHeight());
		ArpD(cdb << ADH << "Tab bounds=" << tabBound
						<< ", frame=" << tabFrame << endl);
		ArpD(cdb << ADH << "mTabWidth=" << mTabWidth
						<< ", mTabHeight=" << mTabHeight << endl);
						
		// Add configuration panels to the tab view.
		for( i=0; i<mConfigViews->size(); i++ ) {
			if( mConfigViews->at(i) ) {
				ArpD(cdb << ADH << "Adding tab #" << i
								<< ", name="
								<< mConfigViews->at(i)->Name() << endl);
				AddTab(mConfigViews->at(i));
				mConfigViews->at(i)->MoveTo(tabBound.left+spacew*2,
											tabBound.top+spaceh*2);
				mConfigViews->at(i)->ResizeTo(tabFrame.Width()-mTabWidth,
											  tabFrame.Height()-mTabHeight);
				ArpD(cdb << ADH << "This panel's frame = "
								<< mConfigViews->at(i)->Frame() << endl);
			}
		}
		
	} catch( ... ) {
		size_t i;
		for( i=0; i<getViews.size(); i++ ) delete getViews[i];
		if( mConfigViews ) {
			for( i=0; i<mConfigViews->size(); i++ ) {
				delete mConfigViews->at(i);
			}
		}
		FreeMemory();
		if( parent ) parent->AddChild(this);
		throw;
	}
	
	if( parent ) parent->AddChild(this);
}

void ArpConfigurePanel::GetPreferredSize(float* width, float* height)
{
	float maxw=0, maxh=0;
	size_t i;
	
	// Get dimensions of all configuration views.
	for( i=0; i<mConfigViews->size(); i++ ) {
		BView* view = 0;
		if( (view=mConfigViews->at(i)) != 0 ) {
			ArpD(cdb << ADH << "Processing dimens for view #" << i
							<< ", name="
							<< view->Name() << endl);
			float vwidth=0, vheight=0;
			// If this view is not attached to the window (i.e., it
			// is not currently displayed by the tab view), then it
			// may not be able to report correct dimensions.  To fix
			// this, we temporarily add it to your view.
			if( !view->Window() ) {
				ArpD(cdb << ADH << "Temporarily attaching view to window."
							<< endl);
				bool hidden = view->IsHidden();
				view->Hide();
				AddChild(view);
				view->GetPreferredSize(&vwidth, &vheight);
				RemoveChild(view);
				if( !hidden ) view->Show();
			} else {
				view->GetPreferredSize(&vwidth, &vheight);
			}
			ArpD(cdb << ADH << "Preferred width=" << vwidth
							<< ", height=" << vheight << endl);
			if( vwidth > maxw ) maxw = vwidth;
			if( vheight > maxh ) maxh = vheight;
		}
	}
	
	ArpD(cdb << ADH << "Final size=(" << (maxw+mTabWidth)
				<< ", " << (maxh+mTabHeight) << ")" << endl);
				
	if( width ) *width = maxw + mTabWidth + 2;
	if( height ) *height = maxh + mTabHeight + 2;
}

void ArpConfigurePanel::AttachedToWindow()
{
	ArpD(cdb << ADH << "ArpConfigurePanel: Attached to window." << endl);
	inherited::AttachedToWindow();
	BView* par = Parent();
	if( par ) SetViewColor(par->ViewColor());
	else SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
}

void ArpConfigurePanel::AllAttached()
{
	ArpD(cdb << ADH << "ArpConfigurePanel: All attached." << endl);
	inherited::AllAttached();
	ArpD(cdb << ADH << "Selecting the first tab..." << endl);
	SendSetPanel(FocusTab());
}

void ArpConfigurePanel::MessageReceived(BMessage *message)
{
	if( !message ) return;
	
	switch( message->what ) {
		case ARP_REBUILD_PANEL_MSG: {
			RebuildPanes();
		} break;
		default: {
			inherited::MessageReceived(message);
		} break;
	}
}

void ArpConfigurePanel::SendSetPanel(int32 tab)
{
	BTab* tabobj = TabAt(tab);
	if( tabobj ) {
		BView* view = tabobj->View();
		if( view ) {
			BMessenger target(view);
			if( target.IsValid() ) {
				BMessage setPanel(ARP_SET_CONFIG_PANEL_MSG);
				setPanel.AddMessenger("panel", BMessenger(this));
				target.SendMessage(&setPanel);
				ArpD(cdb << ADH << "Sent config panel msg: "
						<< setPanel << endl);
			} else {
				ArpD(cdb << ADH << "Invalid messenger for view, "
						<< "couldn't send panel message!" << endl);
			}
		}
	}
}

void ArpConfigurePanel::Select(int32 tab)
{
	inherited::Select(tab);
	
	if( Parent() ) {
		BTab* tabobj = TabAt(tab);
		if( tabobj ) {
			BView* view = tabobj->View();
			BRect frame(Frame());
			if( view ) {
				view->ResizeTo(frame.Width()-mTabWidth,
							   frame.Height()-mTabHeight);
			}
		}
		
		SendSetPanel(tab);
	}
}

void ArpConfigurePanel::Revert()
{
	if( mInitSettings ) {
		const ArpVectorI<ArpConfigurableI*>& configs = mConfig.Configs();
		for( size_t i=0;
			 i<configs.size() && i<mInitSettings->size();
			 i++ ) {
			if( configs[i] && mInitSettings->at(i) ) {
				configs[i]->PutConfiguration(mInitSettings->at(i));
			}
		}
	}
}

// ------------------------------------------------------------------------

// internal message codes
enum {
	DONE_MSG = '.don',
	MAKEDEFAULT_MSG = '.mdf',
	REVERT_MSG = '.rev'
};

ArpConfigureView::ArpConfigureView(BRect frame, const char* name,
								const ArpConfigureFile& config,
								BMessage* message,
								uint32 resizingMode,
								uint32 flags)
	: BView(frame, name, resizingMode, flags),
	  BInvoker(message, 0),
	  mPanel(0),
	  mRevertButton(0), mMakeDefaultButton(0), mDoneButton(0),
	  mPanelFrameWidth(0), mPanelFrameHeight(0),
	  mButtonsWidth(0), mButtonsHeight(0)
{
	const BFont* font = be_plain_font;
	
	font_height fh;
	font->GetHeight(&fh);
	const float spaceh = ceil((fh.ascent+fh.descent+fh.leading)/2);
	const float spacew = ceil(font->StringWidth("W")/2);
	mSpaceWidth = spacew;
	mSpaceHeight = spaceh;
	
	mPanelFrameWidth = spacew;
	mPanelFrameHeight = spaceh;
	
	try {
		float makedefWidth=0, makedefHeight=0;
		if( config.HasFile() ) {
			mMakeDefaultButton = new BButton(frame, "makedefault", "Make Default",
							new BMessage(MAKEDEFAULT_MSG), B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT);
			mMakeDefaultButton->GetPreferredSize(&makedefWidth,&makedefHeight);
			mMakeDefaultButton->ResizeToPreferred();
		}
		
		float revertWidth=0, revertHeight=0;
		mRevertButton = new BButton(frame, "revert", "Revert",
						new BMessage(REVERT_MSG), B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT);
		mRevertButton->GetPreferredSize(&revertWidth,&revertHeight);
		mRevertButton->ResizeToPreferred();
		
		float doneWidth=0, doneHeight=0;
		mDoneButton = new BButton(frame, "done", "Done",
						new BMessage(DONE_MSG), B_FOLLOW_BOTTOM|B_FOLLOW_RIGHT);
		mDoneButton->GetPreferredSize(&doneWidth,&doneHeight);
		mDoneButton->ResizeToPreferred();
		
		mButtonsWidth = revertWidth+makedefWidth+doneWidth+spacew*8;
		mButtonsHeight = revertHeight+spaceh*2;
		
		if( mMakeDefaultButton ) {
			mMakeDefaultButton->MoveTo(frame.right-doneWidth-makedefWidth-spacew*6
								  - (revertWidth>0 ? (revertWidth+spacew):0),
								  frame.bottom-revertHeight-spaceh);
		}
		mRevertButton->MoveTo(frame.right-doneWidth-revertWidth-spacew*4,
							  frame.bottom-revertHeight-spaceh);
		mDoneButton->MoveTo(frame.right-doneWidth-spacew*2,
							frame.bottom-revertHeight-spaceh);
							
		BRect aRect(frame.left, ceil(mPanelFrameHeight/2),
					frame.right,
					frame.Height()-mButtonsHeight-mPanelFrameHeight);
		mPanel = new ArpConfigurePanel(aRect, "Panels", config);
		
		AddChild(mPanel);
		AddChild(mRevertButton);
		if( mMakeDefaultButton ) AddChild(mMakeDefaultButton);
		AddChild(mDoneButton);
		
	} catch(...) {
		delete mRevertButton;
		delete mMakeDefaultButton;
		delete mDoneButton;
		delete mPanel;
	}
}

ArpConfigureView::~ArpConfigureView()
{
}

void ArpConfigureView::GetPreferredSize(float* width, float* height)
{
	float pWidth=0, pHeight=0;
	mPanel->GetPreferredSize(&pWidth, &pHeight);
	//pWidth += mPanelFrameWidth;
	pHeight += mPanelFrameHeight;
	
	if( width ) *width = pWidth > mButtonsWidth ? pWidth : mButtonsWidth;
	if( height ) *height = pHeight + mButtonsHeight;
}

void ArpConfigureView::AttachedToWindow()
{
	ArpD(cdb << ADH << "ArpConfigureView: Attached to window." << endl);
	inherited::AttachedToWindow();
	BView* par = Parent();
	if( par ) SetViewColor(par->ViewColor());
	else SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	mRevertButton->SetTarget(this);
	if( mMakeDefaultButton ) mMakeDefaultButton->SetTarget(this);
	mDoneButton->SetTarget(this);
	//for now, don't do this, as it causes trouble with text
	//controls not seeing changes the user makes
	Window()->SetDefaultButton(mDoneButton);
}

void ArpConfigureView::MessageReceived(BMessage *message)
{
	switch( message->what ) {
		case DONE_MSG: {
			// This little bit of stupidity is to make any
			// text control that has focus to report its changes
			// before we apply the settings.
			BView* lastFocus = Window()->CurrentFocus();
			if (lastFocus) {
				MakeFocus();
				lastFocus->MakeFocus();
			}
			if (mDoneButton) mDoneButton->MakeFocus();
			Invoke();
		} break;
		case MAKEDEFAULT_MSG: {
			Config().WriteSettings();
		} break;
		case REVERT_MSG: {
			if( mPanel ) mPanel->Revert();
		} break;
		default: {
			inherited::MessageReceived(message);
		}
	}
}

void ArpConfigureView::Draw(BRect )
{
	BeginLineArray(6);
	const rgb_color bgcolor = ViewColor();
	
	if( mMakeDefaultButton ) {
		BRect revFrame(mMakeDefaultButton->Frame());
		const float left = revFrame.right + floor((mSpaceWidth*3)/2);
		AddLine( BPoint(left, revFrame.top),
				 BPoint(left, revFrame.bottom),
				 tint_color(bgcolor, B_DARKEN_4_TINT) );
		AddLine( BPoint(left+1, revFrame.top),
				 BPoint(left+1, revFrame.bottom),
				 tint_color(bgcolor, .2) );
	}
	
	BRect bnd(Bounds());
	BRect panFrame(mPanel->Frame());
	AddLine( BPoint(bnd.left, bnd.bottom),
			 BPoint(bnd.left, panFrame.bottom+1),
			 tint_color(bgcolor, .2) );
	AddLine( BPoint(bnd.left, panFrame.bottom+1),
			 BPoint(bnd.right, panFrame.bottom+1),
			 tint_color(bgcolor, .2) );
	AddLine( BPoint(bnd.right, panFrame.bottom+1),
			 BPoint(bnd.right, bnd.bottom),
			 tint_color(bgcolor, B_DARKEN_4_TINT) );
	AddLine( BPoint(bnd.right, bnd.bottom),
			 BPoint(bnd.left, bnd.bottom),
			 tint_color(bgcolor, B_DARKEN_4_TINT) );
	
	EndLineArray();
}
