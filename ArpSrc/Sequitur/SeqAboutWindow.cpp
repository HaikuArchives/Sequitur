/* SeqAboutWindow.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <be/support/Autolock.h>
#include <be/app/MessageRunner.h>
#include <be/interface/Button.h>
#include <be/interface/Screen.h>
#include <be/interface/StringView.h>
#include <be/interface/View.h>
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "Sequitur/SeqAboutWindow.h"

static const float		originX			= 10;
static const float		originY			= 10;
static const float		width			= 430;
static const float		height			= 475;

static const BBitmap*	arp_logo		= NULL;
static const BBitmap*	seq_logo		= NULL;
static const char*		VERSION_STR		= "2.1.1g";

enum {
	kShowWindow = 'swnd'
};

static BLocker gStatusAccess("Status Window");
static SeqAboutWindow* gStatusWindow = NULL;
static bool gStartupFinished = false;

void report_startup_status(const char* msg)
{
	BAutolock _l(gStatusAccess);
	if (!gStartupFinished) {
		if (!gStatusWindow) {
			gStatusWindow = new SeqAboutWindow(true);
			gStatusWindow->Hide();
			gStatusWindow->Show();
		}
		if (gStatusWindow->Lock()) {
			gStatusWindow->SetStartupStatus(msg);
			gStatusWindow->Unlock();
		}
	}
}

void finish_startup_status()
{
	BAutolock _l(gStatusAccess);
	gStartupFinished = true;
	if (gStatusWindow && gStatusWindow->Lock()) {
		gStatusWindow->Quit();
		gStatusWindow = NULL;
	}
}

/*************************************************************************
 * _SEQ-ABOUT-VIEW
 *************************************************************************/
class _SeqAboutView : public BView
{
public:
	_SeqAboutView();

	virtual void	Draw(BRect clip);
	void DrawOn(BView* view, BRect clip);

private:
	typedef BView	inherited;
};

/*************************************************************************
 * _SEQ-CREDITS-VIEW
 *************************************************************************/
class _SeqCreditsView : public BView
{
public:
	_SeqCreditsView(BRect frame);

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect clip);
	void DrawOn(BView* view, BRect clip);

private:
	typedef BView	inherited;

	void			DrawRow(BView* view, BRect clip, float col1, float col2, float top, 
							const char* name, const char* email, const char* descr);
};

/*************************************************************************
 * SEQ-ABOUT-WINDOW
 *************************************************************************/
SeqAboutWindow::SeqAboutWindow(bool startupMode)
		: inherited(BRect(originX, originY, originX, originY),
					startupMode ? "Initializing..." : "About",
					B_MODAL_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
		  mStartupMode(startupMode), mStartupRunner(NULL), mStatusText(NULL)
{
	AddViews();
	BView*	view;
	float	w = 0, h = 0;
	for( view = ChildAt(0); view; view = view->NextSibling() ) {
		BRect	f = view->Frame();
		if( f.right > w ) w = f.right;
		if( f.bottom > h ) h = f.bottom;
	}
	ResizeTo( w, h );

	BScreen	s(this);
	if( s.IsValid() ) {
		BRect	sf = s.Frame();
		float	newX = originX, newY = originY;
		if( w < sf.Width() ) newX = (sf.Width() - w) / 2;
		if( h < sf.Height() ) newY = (sf.Height() - h) / 2;
		MoveTo( newX, newY );
	}
	
	if (mStartupMode) {
		BMessage msg(kShowWindow);
		mStartupRunner = new BMessageRunner(BMessenger(this), &msg, 500000, 1);
	}
}

SeqAboutWindow::~SeqAboutWindow()
{
	delete mStartupRunner;
}

void SeqAboutWindow::MessageReceived(BMessage* msg)
{
	if( msg->what == 'quit' ) {
		Quit();
		return;
	} else if( msg->what == kShowWindow ) {
		if (IsHidden()) Show();
		return;
	}
	inherited::MessageReceived(msg);
}

void SeqAboutWindow::SetStartupStatus(const char* msg)
{
	if (mStatusText) {
		printf("Setting startup status: %s\n", msg);
		mStatusText->SetText(msg);
	}
}

void SeqAboutWindow::AddViews()
{
	BView*	bg = new _SeqAboutView();
	if (!bg) return;
	AddChild(bg);
	_SeqCreditsView*	sc = new _SeqCreditsView( BRect(100, 125, 350, 296) );
	if (sc) bg->AddChild(sc);

	if (!mStartupMode) {
		BButton*	button = new BButton( BRect(width - 55, height - 30, width - 6, height - 10), "OK", "OK", new BMessage('quit') );
		if (button) {
			bg->AddChild(button);
			button->MakeDefault(true);
		}
	} else {
		float		fh = arp_get_font_height(bg);
		mStatusText = new BStringView(BRect(10, height - 10 - fh, width - 10, height - 10), "status", "");
		if (mStatusText) {
			bg->AddChild(mStatusText);
		}
	}
}

/*************************************************************************
 * _SEQ-ABOUT-VIEW
 *************************************************************************/
static bool show_planet()
{
	float		freq = 10;

	if (freq <= 0) return false;
	if (freq >= 100) return true;

	srand( (int32)(system_time()/100) );
	int32		percent = rand() % 100;

	if (percent < freq) return true;
	else return false;
}

_SeqAboutView::_SeqAboutView()
		: inherited( BRect(0, 0, width, height), "bg", B_FOLLOW_NONE, B_WILL_DRAW )
{
	if (!arp_logo) {
		if (show_planet()) arp_logo = ImageManager().FindBitmap("ARP Planet");
		else arp_logo = ImageManager().FindBitmap("ARP Logo");
	}
	if (!seq_logo) seq_logo = ImageManager().FindBitmap("Sequitur Logo");
	SetViewColor( 180, 180, 180 );
}

void _SeqAboutView::Draw(BRect clip)
{
	inherited::Draw(clip);
	DrawOn(this, clip);
}

void _SeqAboutView::DrawOn(BView* view, BRect clip)
{
	float	top = 292;
	float	arpL = 120;
	if (arp_logo) {
		drawing_mode	mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

		view->DrawBitmapAsync(arp_logo, BPoint(arpL, top) );
		top += arp_logo->Bounds().Height() - 5;
		arpL += 10;

		view->SetDrawingMode(mode);
	}
	if (seq_logo) {
		view->DrawBitmapAsync(seq_logo, BPoint(5, 0) );
	}
	SetLowColor( ViewColor() );
	view->SetHighColor(100, 100, 100);
	view->SetFontSize(10);
	DrawString("http://www.angryredplanet.com", BPoint(arpL, top) );

	view->SetHighColor(0, 0, 0);
	view->SetFontSize(20);
	view->DrawString(VERSION_STR, BPoint(380, 114) );
}

/*************************************************************************
 * _SEQ-CREDITS-VIEW
 *************************************************************************/
_SeqCreditsView::_SeqCreditsView(BRect frame)
		: inherited(frame, "credits", B_FOLLOW_NONE, B_WILL_DRAW)
{
}

void _SeqCreditsView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (Parent() ) SetViewColor(Parent()->ViewColor() );
	SetFontSize(10);
}

void _SeqCreditsView::Draw(BRect clip)
{
	inherited::Draw(clip);
	DrawOn(this, clip);
}

static const char* gName[] = {
	"Eric Hackborn", "Dianne Hackborn", "Daniel Civello", "Jace Cavacini", "Syn.Terra", 0
};

static const char* gEmail[] = {
	"hackborn@angryredplanet.com",	"hackbod@angryredplanet.com", "civello@pacbell.net", "bewine@entermail.net", "dream@aevum.net", 0
};

static const char* gDescr[] = {
	"Design, development, and documentation",	"Design and development", "Development and documentation",	"Folder icons and testing", "Testing", 0
};

void _SeqCreditsView::DrawOn(BView* view, BRect clip)
{
	float	fh = 10;
	const char*	str;
	
	view->SetHighColor(0, 0, 0);
	view->SetLowColor(ViewColor() );

	float	top = fh + 2;
	float	col1 = 5, col2 = 100;
	for (int32 k = 0; (str = gName[k]); k++) {
		DrawRow(view, clip, col1, col2, top, str, gEmail[k], gDescr[k]);
		top += 35;
	}
}

void _SeqCreditsView::DrawRow(	BView* view, BRect clip, float col1, float col2, float top, 
								const char* name, const char* email, const char* descr)
{
	float	fh = 12;

	view->SetHighColor(0, 0, 0);
	view->DrawString( name, BPoint(col1, top) );

	if (email) view->DrawString( email, BPoint(col2, top) );

	view->SetHighColor(100, 100, 100);
	if (descr) view->DrawString( descr, BPoint(col1, top + fh + 1) );
}
