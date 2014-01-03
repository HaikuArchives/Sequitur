/*
 * Copyright (c)1997 by Dianne Hackborn.
 * All rights reserved.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@enteract.com>.
 *
 */

#ifndef LAYOUTTEST_H
#include "LayoutTest.h"
#endif

#ifndef PREFWINDOW_H
#include "PrefWindow.h"
#endif

#ifndef LAYOUTWINDOW_H
#include "TestWindow.h"
#endif

#ifndef TERMEXAMPLE_H
#include "TermExample.h"
#endif

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

#ifndef ARPLAYOUT_ARPRUNNINGBAR_H
#include <ArpLayout/ArpRunningBar.h>
#endif

#ifndef ARPLAYOUT_ARPVIEWWRAPPER_H
#include <ArpLayout/ArpViewWrapper.h>
#endif

#ifndef ARPLAYOUT_VIEWSTUBS_H
#include <ArpLayout/ViewStubs.h>
#endif

#ifndef ARPLAYOUT_ARPSCROLLAREA_H
#include <ArpLayout/ArpScrollArea.h>
#endif

#ifndef ARPKERNEL_ARPABOUTWINDOW_H
#include <ArpKernel/ArpAboutWindow.h>
#endif

#include <interface/MenuItem.h>
#include <support/Autolock.h>

#include <float.h>
#include <malloc.h>

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

ArpMOD();

enum {
	TERM_SETTINGS_MSG = 'term',
	ADD_TEST_MSG = 'addt',
	REMOVE_TEST_MSG = 'remt',
	PREFERENCES_MSG = 'sprf'
};

/* ---------------------------------------------------------------
 *
 * class TestView: A simple subclass of ArpLView, which draws some
 * cheesy stuff to show how it is resizing.
 *
 */
 
class TestView : public ArpLayoutView {

  public:
	TestView(char *name); 
	virtual	void	AttachedToWindow();
	virtual void	ComputeDimens(ArpDimens& cur_dimens);
	virtual	void	Draw(BRect updateRect);
};

TestView::TestView(char *name)
	: ArpLayoutView(name, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
{
	//SetViewColor(B_TRANSPARENT_32_BIT);
}

void TestView::AttachedToWindow()
{
	ArpD(cdb << ADH << "TestView::AttachedToWindow() " << Name() << endl);
}

void TestView::ComputeDimens(ArpDimens& cur_dimens)
{
	float fw = BasicFont()->StringWidth("W");
	font_height fhs;
	BasicFont()->GetHeight(&fhs);
	float fh = fhs.ascent+fhs.descent+fhs.leading;
	
	cur_dimens.X().SetTo(0, fw, fw*2, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, fh, fh*2, ArpAnySize, 0);
}

void TestView::Draw(BRect /*updateRect*/)
{
	ArpD(cdb << ADH << "TestView::Draw() " << Name() << endl);
	BRect frm = Bounds();
	StrokeRect(frm);
	StrokeLine(BPoint(frm.left,frm.top),
				BPoint(frm.right,frm.bottom));
	StrokeLine(BPoint(frm.right,frm.top),
				BPoint(frm.left,frm.bottom));
}

/* ---------------------------------------------------------------
 *
 * class TestWindow: Out main application window.
 *
 */

#if 0
static void print_ltree(ArpLayoutable* root, int indent)
{
	char ibuff[100];
	int32 i;
	if( indent > 98 ) indent=0;
	for( i=0; i<indent; i++ ) ibuff[i] = ' ';
	ibuff[i] = 0;
	
	if( !root ) {
		printf("%s<NULL>\n",&ibuff[0]);
		return;
	}
	printf("%s%s\n",&ibuff[0],class_name(root));
	
	int num = root->CountLayoutChildren();
	for( i=0; i<num; i++ ) {
		print_ltree(root->LayoutChildAt(i),indent+2);
	}
}

static void print_vtree(BView* root, int indent)
{
	char ibuff[100];
	int32 i;
	if( indent > 98 ) indent=0;
	for( i=0; i<indent; i++ ) ibuff[i] = ' ';
	ibuff[i] = 0;
	
	if( !root ) {
		printf("%s<NULL>\n",&ibuff[0]);
		return;
	}
	printf("%s%s\n",&ibuff[0],class_name(root));
	
	int num = root->CountChildren();
	for( i=0; i<num; i++ ) {
		print_vtree(root->ChildAt(i),indent+2);
	}
}
#endif

TestWindow::TestWindow(BApplication* myApp)
	: BWindow(BRect(20,20,100,100), "ArpLayout Test", B_DOCUMENT_WINDOW, 0),
	  root(0), addpos(0)
{
	// NOTE!!
	// This is only one POSSIBLE way to create a layout hierarchy.
	// I use it because I happen to like it, but you don't need to.
	// You could also just create the objects individual and attach
	// them to each other as traditionally done with BViews; and
	// in the future, you will be able to flatten them into a BMessage
	// and reconstruct the layout hierarchy.
	
	// The layout engine also fully supports dynamically adding and
	// removing objects from an active hierarchy, so a tool for
	// creating control layouts should be possible to write.
	
	// Anyway...
	
	// First, create the window's UI layout.  While doing so,
	// here are places to put some various UI objects that
	// we'll later want to manipulate outside of the layout
	// definition...
	ArpMenuBar* mainmenu = 0;
	ArpScrollArea* sarea = 0;
	ArpOutlineListView* mylist = 0;
	
	// Here is a standard set of parameters that we will apply to the
	// TestView class defined above.  "SetIndirect" adds an "indirect"
	// parameter to the message -- it is really just a \0-terminated
	// string, but stored with a custom type.  When the layout engine
	// sees this, it will retrieve the value for this parameter from
	// from the global name space; if that global parameter doesn't
	// exist, it is automatically created.  Thus these two global
	// global parameters -- BoxBackColor and BoxTextColor -- will
	// automatically show up in the preferences view; changing them will
	// affect the BackgroundColor and ForegroundColor, respective, of
	// any views that use this association.
	ArpMessage box_params;
	box_params
		.SetIndirect("BackgroundColor","BoxBackColor")
		.SetIndirect("ForegroundColor","BoxTextColor");
	
	// Another set of standard parameters that are applied to a
	// different TestView instance.  FillBackColor and FillTextColor
	// are standard global values, which are the colors used to fill
	// data regions like list views and text areas.
	ArpMessage fill_params;
	fill_params
		.SetIndirect("BackgroundColor","FillBackColor")
		.SetIndirect("ForegroundColor","FillTextColor");
	
	ArpD(cdb << ADH << "Initial Window Bounds: " << Bounds() << endl);
	
	// Now the layout definition itself: this supplies everything
	// needed to determine how to position and resize the UI.
	// Notice, not a single pixel in sight!
	
	// First, ArpRootLayout is the top of the layout hierarchy.  It
	// is a BView and ArpLayoutable, which defines global information
	// about the interior layout: the layout conforms to the size of
	// this BView, and it contains the BMessage with the global
	// parameter set.
	
	// Note an import thing here: the layout does not care at all about
	// the surrounding window, so you can use the ArpLayoutable classes
	// to embed controls in a larger application that only knows about
	// raw BView objects.  The ArpRootLayout class overrides its
	// GetPreferredSize() method to return the resulting information
	// about the underlying layout.
	
	(root = new ArpRootLayout(Bounds(), "Root"))
	
	// The first thing underneath the root object is an ArpRunningBar
	// with a vertical orientation.  This positions its children one on
	// top of the other, the first one on top.
->AddLayoutChild((new ArpRunningBar("TopVBar"))

	// Set a parameter for the ArpRunningBar: its orientation should
	// be vertical.
	->SetParams(ArpMessage()
		.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
	)
	
	// Add the first child to the running bar: this is the standard
	// menu structure at the top of the window.
	->AddLayoutChild((mainmenu = new ArpMenuBar("MainMenu"))
	
		// Set the constraints for the menu bar.  Constraints are like
		// parameters, except they are used by the parent widget to
		// determine how to layout its children; here, we say to set
		// its weight to zero, which tells the running bar that its
		// height (in the B_VERTICAL orientation) should not grow to
		// use any extra space.
		//
		// An ArpLayoutable object can contain any arbitrary constraints,
		// although only the ones its parent knows about will be used.
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
	)
	
	// Below the menu bar is an instance of our TestView BView.  We set
	// its weight such that it will get some of any extra space that the
	// parent ArpRunningBar has.
	->AddLayoutChild((new TestView("Test"))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,1)
		)
	)
	
	// Another child; here, just a raw ArpLayoutable, which will simply
	// result in a "gap" between the children before and after it.
	->AddLayoutChild((new ArpLayout("Fill"))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,1)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
	)
	
	// The next child is another ArpRunningBar with a vertical orientation.
	->AddLayoutChild((new ArpRunningBar("TestVBar"))
		->SetParams(ArpMessage()
			.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,1)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
		
		// Add the first child to this new running bar: another TestView.
		// Here we set the box_params parameters that we defined above,
		// giving it a custom coloring.
		->AddLayoutChild((new TestView("Test1"))
			->SetParams(box_params)
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
			)
		)
		
		// And now a horizontal running bar, which will place its children
		// across the bottom of the previous child.
		->AddLayoutChild((new ArpRunningBar("TestHBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
			)
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
			)
			
			// Plus three test views into this running bar.
			->AddLayoutChild((new TestView("Test2"))
				->SetParams(box_params)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
				)
			)
			->AddLayoutChild((new TestView("Test3"))
				->SetParams(fill_params)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
				)
			)
			->AddLayoutChild((new TestView("Test4"))
				->SetParams(box_params)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
				)
			)
		)
	)
	
	// We're back up to the level of the first running bar.  The
	// next child is a horizontal running bar, which contains the
	// three buttons.  A new running bar constraint is used here,
	// "FillC", which controls how the object places its children
	// within their assigned space.  Normally, if the maximum size
	// of a child is larger than the space assigned to it, it will
	// be made that maximum size and centered in the space.  FillC
	// lets us do other things.
	->AddLayoutChild((new ArpRunningBar("ButtonHBar"))
		->SetParams(ArpMessage()
			.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
		->AddLayoutChild((new ArpButton("Terminal","Terminal" B_UTF8_ELLIPSIS,
							new BMessage(TERM_SETTINGS_MSG)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpCenter)
			)
		)
		->AddLayoutChild((new ArpButton("Add","Add",
							new BMessage(ADD_TEST_MSG)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpCenter)
			)
		)
		->AddLayoutChild((new ArpButton("Remove","Remove",
							new BMessage(REMOVE_TEST_MSG)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpCenter)
			)
		)
	)
	
	// Finally, place the scrolling OutlineListView at the bottom
	// of the window.  This is contained in a ArpScrollArea, which
	// is a custom implementation of BScrollArea the works better
	// with the layout system -- the BScrollArea doesn't report any
	// useful information about the dimensions of its children.
	->AddLayoutChild((sarea = new ArpScrollArea("ScrollArea"))
		->SetParams(ArpMessage()
			.SetBool(ArpScrollArea::ScrollHorizontalP,true)
			.SetBool(ArpScrollArea::ScrollVerticalP,true)
			.SetBool(ArpScrollArea::InsetCornerP,true)
			.SetInt32(ArpScrollArea::BorderStyleP,B_NO_BORDER)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,1)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
		->AddLayoutChild((mylist = new ArpOutlineListView(
						"MyList",B_SINGLE_SELECTION_LIST))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
			)
		)
	)
);

	// Find the position above which our buttons will add children.
	addpos = root->FindLayoutable("ButtonHBar");
	
	// Place some items into the top menu bar.
	if( mainmenu ) {
		BMenu* first = new BMenu("File");
		BMenuItem *item; 
	
		item = new BMenuItem("About LayoutTest" B_UTF8_ELLIPSIS, 
	                             new BMessage(B_ABOUT_REQUESTED)); 
		item->SetTarget(myApp); 
		first->AddItem(item);
	
		item = new BMenuItem("Preferences" B_UTF8_ELLIPSIS, 
	                             new BMessage(PREFERENCES_MSG)); 
		item->SetTarget(this); 
		first->AddItem(item);
	
		item = new BMenuItem("Quit",
								new BMessage(B_QUIT_REQUESTED)); 
		item->SetTarget(myApp);
		first->AddItem(item);
	
		mainmenu->AddItem(first);
		mainmenu->SetFlags(mainmenu->Flags()|B_WILL_DRAW);
	}
	
	// Put some stuff into the list view.
	if( mylist ) {
		mylist->AddItem(new BStringItem("This is an example of the ArpLayout library."));
		mylist->AddItem(new BStringItem("Play with the window; be sure to enlarge and shrink it!"));
		mylist->AddItem(new BStringItem("Other things to do:"));
		mylist->AddItem(new BStringItem("Another window"));
		mylist->AddItem(new BStringItem("Press the \"Terminal\" button to see a more",1));
		mylist->AddItem(new BStringItem("complicated window.",1));
		mylist->AddItem(new BStringItem("(No reward for guessing where it came from!",1));
		mylist->AddItem(new BStringItem("Dynamic control"));
		mylist->AddItem(new BStringItem("Press the \"Add\" and \"Remove\" button to",1));
		mylist->AddItem(new BStringItem("dynamically add and remove objects into the layout",1));
		mylist->AddItem(new BStringItem("hierarchy.",1));
		mylist->AddItem(new BStringItem("Parameters"));
		mylist->AddItem(new BStringItem("Select the \"Preferences\" menu item.",1));
		mylist->AddItem(new BStringItem("This allows you to manipulate the global",1));
		mylist->AddItem(new BStringItem("parameters of a layout, controlling font and",1));
		mylist->AddItem(new BStringItem("color attributes.  There are a number of",1));
		mylist->AddItem(new BStringItem("standard global values, and others may be easily",1));
		mylist->AddItem(new BStringItem("added by objects (such as the boxes, here) or",1));
		mylist->AddItem(new BStringItem("as a layout is constructed.",1));
	}
	
	// And finally attach the layout to the window, which will
	// compute the top-level dimensions.
	if( root ) {
		ArpD(cdb << ADH << "Before attach: win=" << Bounds()
						<< ", root=" << root->Bounds() << endl);
		this->AddChild(root);
		ArpD(cdb << ADH << "After attach: win=" << Bounds()
						<< ", root=" << root->Bounds() << endl);
		
		float w=100, h=00;
		root->GetPreferredSize(&w, &h);
		MoveTo(30,30);
		ResizeTo(w, h);
		
		ArpD(cdb << ADH << "After resize: win=" << Bounds()
						<< ", root=" << root->Bounds() << endl);
						
#if 0
		printf("LAYOUT TREE:\n");
		print_ltree(root,2);
		printf("VIEW TREE:\n");
		print_vtree(root,2);
#endif
	}
}

TestWindow::~TestWindow()
{
}

void TestWindow::MakeTermSettings()
{
	BView* settings = 0;
	BView* root = 0;
	BWindow* win = 0;
	
	try {
		// Pick some arbitrary initial frame for the window.
		BRect initFrame(0,0,100,100);
		
		// Create our three objects: the settings view, the top-level view
		// in the window, and the window itself.
		settings = GetTermSettings(BMessenger(), BMessage());
		root = new BView(initFrame, "root", B_FOLLOW_ALL, B_WILL_DRAW);
		win = new BWindow(initFrame, "Terminal Settings",
							B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
							B_ASYNCHRONOUS_CONTROLS);
		
		// Set the background color of the root view and add it to the window.
		root->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		win->AddChild(root);
		
		// Set the size and position of the settings view -- inset a little
		// from the root -- and add it.
		float fw = be_plain_font->StringWidth("W");
		font_height fhs;
		be_plain_font->GetHeight(&fhs);
		float fh = fhs.ascent+fhs.descent+fhs.leading;
		
		settings->MoveTo(fw, fh);
		settings->ResizeTo(initFrame.right-fw*2, initFrame.bottom-fh*2);
		root->AddChild(settings);
		
		win->Run();
		mTermWin = win;
		mTermSet = settings;
		
		if( root ) {
			// Send a message to the view with our current global settings.
			// If the view is an ArpRootView, it will get the message and
			// know what to do with it.
			const BMessage* curGlobs = this->root->Globals()->GlobalValues();
			if( curGlobs ) {
				BMessage globals(*curGlobs);
				globals.what = ARP_PREF_MSG;
				BMessage ret;
				mTermSet.SendMessage(&globals, &ret);
			}
		}
		
		// Get preferred size of settings view, and make final window dimensions
		// from this.
		float width=0, height=0;
		settings->GetPreferredSize(&width, &height);
		width += fw*2;
		height += fh*2;
		
		float minw=0,minh=0,maxw=0,maxh=0;
		win->GetSizeLimits(&minw,&maxw,&minh,&maxh);
		win->SetSizeLimits(width,maxw,height,maxh);
		
		BRect frm = Frame();
		win->ResizeTo(width, height);
		BRect cfrm = Frame();
		win->MoveTo( frm.left
					+ (frm.Width()-cfrm.Width())/2,
				 	frm.top
				 	+ (frm.Height()-cfrm.Height())/2);
		
		win->Show();
		mTermWin = win;
		mTermSet = settings;
	} catch(...) {
		delete settings;
		delete root;
		delete win;
	}
}

void TestWindow::MessageReceived(BMessage *message)
{
	if( message ) {
		switch(message->what) {
			case TERM_SETTINGS_MSG: {
				if( !mTermWin.IsValid() ) {
					MakeTermSettings();
				}
			} break;
			case ADD_TEST_MSG: {
				if( addpos && addpos->LayoutParent() ) {
					TestView* newView = new TestView("AddedTest");
					newView->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
					);
					addpos->LayoutParent()->AddLayoutChild(newView, ArpNoParams,
														   addpos);
					root->SetWindowLimits();
				}
			} break;
			case REMOVE_TEST_MSG: {
				ArpBaseLayout* rempos = addpos ? addpos->PreviousLayoutSibling() : 0;
				if( rempos && rempos->LayoutName() ) {
					if( strcmp(rempos->LayoutName(), "AddedTest") == 0 ) {
						rempos->LayoutRemoveSelf();
						delete rempos;
						root->SetWindowLimits();
					}
				}
			} break;
			case ARP_PREF_MSG: {
				if( root ) root->UpdateGlobals(message);
				// Because we are just using a generic BWindow, the object
				// won't correctly handle changes to its global settings.
				//if( mTermSet.IsValid() ) mTermSet.SendMessage(message);
			} break;
			case PREFERENCES_MSG: {
				if( !mPrefWin.IsValid() ) {
					PrefWindow*	pWindow
						= new PrefWindow(BMessenger(this),Globals()->GlobalValues());
					if( pWindow ) {
						mPrefWin = pWindow;
						pWindow->Show();
					}
				}
			}
			default:
				inherited::MessageReceived(message);
		}
	}
}

void TestWindow::FrameResized(float width,float height)
{
	inherited::FrameResized(width, height);
}

bool TestWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

TestApplication::TestApplication()
	: BApplication("application/x-vnd.ARP.LayoutTest")
{
	ArpD(cdb << ADH << "LayoutTest application started, creating window." << endl);
	TestWindow*	tWindow = new TestWindow(this);
	if( tWindow ) {
		tWindow->Show();
#if 0
		PrefWindow*	pWindow
			= new PrefWindow(BMessenger(tWindow),tWindow->Globals());
		if( pWindow ) pWindow->Show();
		LayoutWindow* lWindow
			= new LayoutWindow();
		if( lWindow ) lWindow->Show();
#endif
	}
}

void TestApplication::ArgvReceived(int32 argc, char** argv)
{
	ArpParseDBOpts(argc, argv);
}

void TestApplication::AboutRequested(void)
{
	if( !mAboutWin.IsValid() ) {
		ArpAboutWindow* win = 
			new ArpAboutWindow(0, "LayoutTest", PROGRAM_VERSION, __DATE__,
					"ArpLayout test app version " PROGRAM_VERSION
					" / " __DATE__ "\n"
					B_UTF8_COPYRIGHT "1998 Angry Red Planet, Inc.\n\n"
					
					"This is a simple example of the ArpLayoutable\n"
					"library classes.\n\n"
					
					"For more info and the latest version, see\n"
					"<URL:http://www.angryredplanet.com/>.\n\n"
					
					"Written by Dianne Hackborn\n"
					"(email: hackbod@angryredplanet.com)\n\n");

		mAboutWin = BMessenger(win);
		win->Show();
	}
}

static void mymallocerror(enum mcheck_status state)
{
	switch(state) {
		case MCHECK_DISABLED:
			printf("**** MALLOC ERROR: MCHECK_DISABLED\n");
			break;
		case MCHECK_OK:
			printf("**** MALLOC ERROR: MCHECK_OK\n");
			break;
		case MCHECK_FREE:
			printf("**** MALLOC ERROR: MCHECK_FREE\n");
			break;
		case MCHECK_HEAD:
			printf("**** MALLOC ERROR: MCHECK_HEAD\n");
			break;
		case MCHECK_TAIL:
			printf("**** MALLOC ERROR: MCHECK_TAIL\n");
			break;
		default:
			printf("**** MALLOC ERROR: UNKNOWN CODE #%d\n",state);
			break;
	}
}

#if 0
static void (*oldfree)(void *ptr) = NULL;
static void *(*oldmalloc)(size_t size) = NULL;

static void myfree(void* ptr)
{
	printf("DEBUG: free(%x)\n",ptr);
	if( oldfree ) (*oldfree)(ptr);
	printf("Can't find original free hook!\n");
	fflush(stdout);
	abort();
}

static void* mymalloc(size_t size)
{
	printf("malloc(%d) = ",size);
	if( oldmalloc ) {
		void* mem = (*oldmalloc)(size);
		printf("%x\n",mem);
		return mem;
	}
	printf("Can't find original free hook!\n");
	fflush(stdout);
	return NULL;
}
#endif

main()
{	
	#if 0
	oldfree = __free_hook;
	__free_hook = &myfree;
	oldmalloc = __malloc_hook;
	__malloc_hook = &mymalloc;
	#endif
	//mcheck(&mymallocerror);
	//mtrace();
	
	TestApplication *myApplication;

	myApplication = new TestApplication();
	myApplication->Run();
	
	delete(myApplication);
	return(0);
}
