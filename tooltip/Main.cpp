#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _MESSAGE_RUNNER_H
#include <app/MessageRunner.h>
#endif

#ifndef _BITMAP_H
#include <interface/Bitmap.h>
#endif

#ifndef _PICTURE_H
#include <interface/Picture.h>
#endif

#ifndef _SCREEN_H
#include <interface/Screen.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#include <support/Debug.h>
#include <support/String.h>

#ifndef MAIN_H
#include "Main.h"
#endif

#include <cstdlib>
#include <cstdio>
#include <getopt.h>

const char* AppSig = "application/x-vnd.Be.ToolTIp";

static rgb_color
mix_color(rgb_color color1, rgb_color color2, float portion)
{
	rgb_color ret;
	ret.red = uint8(color2.red*portion + color1.red*(1-portion) + .5);
	ret.green = uint8(color2.green*portion + color1.green*(1-portion) + .5);
	ret.blue = uint8(color2.blue*portion + color1.blue*(1-portion) + .5);
	ret.alpha = uint8(color2.alpha*portion + color1.alpha*(1-portion) + .5);
	return ret;
}

static void Usage(int /*argc*/, char **argv)
{
	printf("Usage: %s <file>\n", argv[0]);
}

int main(int argc, char **argv)
{
	int c;
	
	// Parse options
	while(1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"help",	no_argument,	0,	'h'},
			{0,0,0,0}
		};
		c = getopt_long (argc, argv, "h", long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case 'h':
			case '?':
				Usage(argc,argv);
				return 0;
			default:
				printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}
	
	MainApp myApplication;

	myApplication.Run();
	
	return(0);
}

MainApp::MainApp()
	: BApplication(AppSig),
	  mCmdLine(false)
{
}

MainApp::~MainApp()
{
}

bool MainApp::QuitRequested(void)
{
	if( mTipWin.IsValid() ) mTipWin.SendMessage(B_QUIT_REQUESTED);
	if( mMainWin.IsValid() ) {
		mMainWin.SendMessage(B_QUIT_REQUESTED);
	}
	return true;
}

void MainApp::AboutRequested(void)
{
#if 0
	if( !mAboutWin.IsValid() ) {
		ArpAboutWindow* win = 
			new ArpAboutWindow(0, "printmsg", 0, __DATE__,
					"printmsg version " PROGRAM_VERSION
					" / " __DATE__ "\n"
					B_UTF8_COPYRIGHT "1999 Angry Red Planet Software.\n\n"
					
					"Print BMessage objects.  Wow.\n\n"
					
					"For more info and the latest version, see\n"
					"<URL:http://www.angryredplanet.com/>.\n\n"
					
					"Written by Dianne Hackborn\n"
					"(email: hackbod@angryredplanet.com)\n\n");

		mAboutWin = BMessenger(win);
		win->Show();
	}
#endif
}

void MainApp::DispatchMessage(BMessage *message,
								  BHandler *handler)
{
	inherited::DispatchMessage(message,handler);
}

void MainApp::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	switch( message->what ) {
		default:
			inherited::MessageReceived(message);
	}
}

class PrintView : public BView
{
public:
	PrintView(	BRect frame,
				const char *name,
				uint32 resizeMask = B_FOLLOW_ALL,
				uint32 flags = B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
		: BView(frame, name, resizeMask, flags)
	{
		SetFont(be_bold_font);
	}
	
	virtual	void MessageReceived(BMessage *msg)
	{
		if( !msg ) return;
		if( msg->WasDropped() ) {
			printf("Message dropped on window:\n");
			msg->PrintToStream();
		}
		
		BView::MessageReceived(msg);
	}
	
	virtual	void Draw(BRect updateRect)
	{
		MoveTo(BPoint(0, Bounds().bottom/2));
		SetFont(be_bold_font);
		DrawString("Drop Me, Baby!");
	}
};

class TipView : public BView
{
public:
	TipView(BRect frame, const char* name,
			uint32 resizeMask = B_FOLLOW_NONE,
			uint32 flags = B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
		: BView(frame, name, resizeMask, flags),
		  fTip("This is a TIP.")
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(255, 255, 0);
	}
	
	virtual	void Draw(BRect updateRect)
	{
		static rgb_color shine = { 255, 255, 255, 255 };
		static rgb_color shadow = { 0, 0, 0, 255 };
		static rgb_color text = { 0, 0, 0, 255 };
		
		BRect b(Bounds());
		
		SetHighColor(mix_color(LowColor(), shine, .5));
		StrokeLine(BPoint(0, b.bottom), BPoint(0, 0));
		StrokeLine(BPoint(0, 0), BPoint(b.right, 0));
		SetHighColor(mix_color(LowColor(), shadow, .5));
		StrokeLine(BPoint(b.right, 0), BPoint(b.right, b.bottom));
		StrokeLine(BPoint(b.right, b.bottom), BPoint(1, b.bottom));
		
		FillRect(BRect(1, 1, b.right-1, b.bottom-1), B_SOLID_LOW);
		
		font_height fh;
		GetFontHeight(&fh);
		SetHighColor(text);
		DrawString(fTip.String(), BPoint(1, fh.ascent));
	}

	virtual void GetPreferredSize(float* width, float* height)
	{
		font_height fh;
		GetFontHeight(&fh);
		*height = fh.ascent+fh.descent + 2;
		*width = StringWidth(fTip.String()) + 2;
	}
	
private:
	BString fTip;
};

static void mix_bitmaps(BBitmap* out, BBitmap* b1, BBitmap* b2, uint8 amount)
{
	if( out->BitsLength() != b1->BitsLength() ||
		out->BitsLength() != b2->BitsLength() ||
		out->ColorSpace() != b1->ColorSpace() ||
		out->ColorSpace() != b2->ColorSpace() ) {
		TRESPASS();
		return;
	}
	
	switch( out->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		{
			uint8* sOut = (uint8*)out->Bits();
			uint8* eOut = sOut + out->BitsLength();
			uint8* sB1 = (uint8*)b1->Bits();
			uint8* sB2 = (uint8*)b2->Bits();
			
			while( sOut < eOut ) {
				*sOut = (uint8)( ( ((uint16)*sB1)*(255-amount)
								 + ((uint16)*sB2)*(amount)
								 ) / 255 );
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		default:
			TRESPASS();
	}
}

class TipWindow : public BWindow
{
public:
	TipWindow()
		: BWindow(BRect(-100, -100, -90, -90),
				  "Tool Tip",
				  B_NO_BORDER_WINDOW_LOOK,
				  B_FLOATING_ALL_WINDOW_FEEL,
				  B_AVOID_FOCUS|B_NO_WORKSPACE_ACTIVATION
				  |B_WILL_ACCEPT_FIRST_CLICK|B_ASYNCHRONOUS_CONTROLS),
		  fDrawer(Bounds(), "drawer", B_FOLLOW_ALL, B_WILL_DRAW),
          fRunner(0), fAnim(0), fTip(0),
          fBackPic(0), fForePic(0), fMixPic(0)
	{
		fDrawer.SetViewColor(B_TRANSPARENT_COLOR);
		AddChild(&fDrawer);
		//Minimize(true);
		Run();
		//Show();
		ShowTip();
	}
	~TipWindow()
	{
		fDrawer.RemoveSelf();
		delete fRunner;
		delete fAnim;
		delete fForePic;
		delete fBackPic;
		delete fMixPic;
	}
	
	virtual void MessageReceived(BMessage* msg)
	{
		switch( msg->what ) {
			case 'anim':
				//printf("--> Animate.\n");
				fCurAlpha += .05;
				if( fCurAlpha >= fDestAlpha ) {
					delete fAnim;
					fAnim = 0;
					delete fBackPic;
					fBackPic = 0;
					delete fForePic;
					fForePic = 0;
					printf("fAnim=%p, fBackPic=%p, fForePic=%p\n",
							fAnim, fBackPic, fForePic);
					if( fTip->Window() ) {
						fTip->RemoveSelf();
					}
					fDrawer.AddChild(fTip);
					return;
				}
				
				//printf("Drawing with alpha=%f, tipWin=%p\n",
				//		fCurAlpha, fTip->Window());
				mix_bitmaps(fMixPic, fBackPic, fForePic,
							(uint8)(255*fCurAlpha+.5));
				fDrawer.DrawBitmap(fMixPic);
				#if 0
				fDrawer.PushState();
				fDrawer.SetDrawingMode(B_OP_COPY);
				fDrawer.DrawBitmap(fBackPic);
				fDrawer.SetDrawingMode(B_OP_ALPHA);
				fDrawer.SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
				fDrawer.SetHighColor(0, 0, 0, (uchar)(255*fCurAlpha));
				//fDrawer.FillRect(fDrawer.Bounds());
				fDrawer.DrawBitmap(fForePic);
				fDrawer.PopState();
				fDrawer.Sync();
				#endif
				break;
				
			case 'puls':
				//printf("--> Pulse.\n");
				ShowTip();
				break;
				
			default:
				inherited::MessageReceived(msg);
				break;
		}
	}
	
	void ShowTip()
	{
		Lock();
		if( !IsHidden() ) Hide();
		//if( !IsMinimized() ) Minimize(true);
		
		delete fRunner;
		fRunner = 0;
		delete fAnim;
		fAnim = 0;
		
		printf("Start show with %ld children\n", CountChildren());
		if( fTip ) {
			fTip->RemoveSelf();
			delete fTip;
			fTip = 0;
		}
		
		printf("Now have %ld children\n", CountChildren());
		
		delete fBackPic;
		fBackPic = 0;
		delete fForePic;
		fForePic = 0;
		delete fMixPic;
		fMixPic = 0;
		
		fTip = new TipView(Bounds(), "TipView");
		float w, h;
		fTip->GetPreferredSize(&w, &h);
		ResizeTo(w, h);
		fTip->ResizeTo(w, h);
		
		BScreen s(this);
		BRect sb(s.Frame());
		float x = floor(sb.left + (sb.right-w)*drand48() + .5);
		float y = floor(sb.top + (sb.bottom-h)*drand48() + .5);
		
		MoveTo(x, y);
		
		fRunner = new BMessageRunner(BMessenger(this),
									 new BMessage('puls'),
									 (bigtime_t)(5*1000*1000*drand48()));
		
		BRect bbnd(Bounds());
		bbnd.bottom++;
		
		BRect wfrm(Frame());
		//printf("Getting screen bitmap frame ");
		wfrm.PrintToStream();
		fBackPic = new BBitmap(bbnd, 0,
							   s.ColorSpace(), B_ANY_BYTES_PER_ROW,
							   s.ID());
		s.ReadBitmap(fBackPic, false, &wfrm);
		//s.GetBitmap(&fBackPic, false, &wfrm);
		if( !fBackPic ) {
			printf("*** ERROR GETTING SCREEN BITMAP.\n");
			Show();
			//Minimize(false);
			return;
		}
		
		fCurAlpha = 0.0;
		fDestAlpha = 1.0;
		
		fForePic = new BBitmap(bbnd,
							   B_BITMAP_CLEAR_TO_WHITE|B_BITMAP_ACCEPTS_VIEWS,
							   fBackPic->ColorSpace(), B_ANY_BYTES_PER_ROW,
							   s.ID());
		fForePic->Lock();
		fForePic->AddChild(fTip);
		fTip->PushState();
		fTip->Draw(fTip->Bounds());
		fTip->PopState();
		fTip->Sync();
		fTip->RemoveSelf();
		fForePic->Unlock();
		
		fMixPic = new BBitmap(bbnd, 0, fBackPic->ColorSpace(),
							  B_ANY_BYTES_PER_ROW, s.ID());
                              
		fAnim = new BMessageRunner(BMessenger(this),
									new BMessage('anim'), 50*1000);
									
		Show();
		Minimize(false);
		
		Unlock();
	}
	
private:
	typedef BWindow inherited;
	
	BView fDrawer;

	BMessageRunner* fRunner;
	BMessageRunner* fAnim;
	TipView* fTip;
	BBitmap* fBackPic;
	BBitmap* fForePic;
	BBitmap* fMixPic;
	float fCurAlpha;
	float fDestAlpha;
};

bool MainApp::MakeMainWindow(const entry_ref file)
{
	BWindow* win = new BWindow(BRect(30,30,400,200), "Drop Message",
							   B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
							   B_ASYNCHRONOUS_CONTROLS|B_QUIT_ON_WINDOW_CLOSE);
	if( !win ) return false;
	
	win->AddChild( new PrintView(win->Bounds(), "PrintView") );
	win->Show();
	mMainWin = BMessenger(win);
	
	win = new TipWindow();
	if( !win ) return false;
	
	mTipWin = BMessenger(win);
	
	return true;
}

void MainApp::ArgvReceived(int32 argc, char** argv)
{
	//ArpParseDBOpts(argc, argv);
}

void MainApp::RefsReceived(BMessage* message)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	
	if( !message ) return;
	
	message->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 
	
	if( count > 0 ) {
		if( message->FindRef("refs", 0, &ref) == B_OK ) {
			if( !mMainWin.IsValid() ) {
				MakeMainWindow(ref);
			}
		}
	}
}

void MainApp::ReadyToRun()
{
	if( !mMainWin.IsValid() ) MakeMainWindow();
}
