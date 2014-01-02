/*
	
	ArpAboutView.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#include "ArpKernel/ArpAboutView.h"

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _SCREEN_H
#include <interface/Screen.h>
#endif

#ifndef _BUTTON_H
#include <interface/Button.h>
#endif

#ifndef _TRANSLATION_UTILS_H
#include <translation/TranslationUtils.h>
#endif

#ifndef _APPLICATION_H
#include <app/Application.h>
#endif

#ifndef _ROSTER_H
#include <app/Roster.h>
#endif

#ifndef _APPFILEINFO_H
#include <storage/AppFileInfo.h>
#endif

#ifndef _FILE_H
#include <storage/File.h>
#endif

#ifndef ARPCOLLECTIONS_ARPSTLVECTOR_H
#include <ArpCollections/ArpSTLVector.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <stdio.h>

ArpMOD();

enum {
	GO_WEB_MSG = '.gwb'
};

ArpAboutView::ArpAboutView(BRect rect, const char *name, const char *text,
						const char* appname, const char* verstr, const char* build)
	   	   : BView(rect, name, B_FOLLOW_ALL,
	   	   			B_WILL_DRAW|B_FRAME_EVENTS|B_FULL_UPDATE_ON_RESIZE),
	   	     mText(0), N(0), mAppPic(0), mArpPic(0), mDoneBut(0), mWebBut(0),
	   	     mBitmap(0), mDrawView(0),
	   	     mStep(0), mAnimBaseTime(system_time()), mFirstTime(true),
	   	     mAnimThread(-1)
{
	try {
		mText = new ArpSTLVector<BString>(0);
		for( int i=0; i<128; i++ ) mUsedChars[i] = false;
		
		ArpStrTok tok(ArpLIT(text), ArpLIT("\r\n"));
		ArpString curStr;
		while( ! (curStr=tok.Next()).IsNull() ) {
			mText->push_back(curStr);
			const char* c = mText->at(mText->size()-1).String();
			ArpD(cdb << ADH << "Got line: " << c << endl);
			while( *c ) {
				while( ((*c)&0x80) != 0 ) c++;
				if( *c ) {
					// This is redundant, but paranoia rules...
					if( *c >= 0 ) mUsedChars[*c] = true;
					c++;
				}
			}
		}
		
		N = mText->size();
		
		mAppName = appname;
		mVersion = verstr;
		mBuild = build;
		
		{
			app_info ai;
			BFile file;
			BAppFileInfo afi;
			be_app->GetAppInfo(&ai);
			file.SetTo(&ai.ref, B_READ_ONLY);
			afi.SetTo(&file);
			mAppPic = new BBitmap(BRect(0,0,31,31), B_CMAP8);
			if( afi.GetIcon(mAppPic, B_LARGE_ICON) != B_OK ) {
				delete mAppPic;
				mAppPic = 0;
			}
			if( mVersion == "" ) {
				version_info vi;
				if( afi.GetVersionInfo(&vi, B_APP_VERSION_KIND) == B_OK ) {
					mVersion << vi.major;
					if( vi.middle > 0 ) {
						mVersion << ".";
						mVersion << vi.middle;
						if( vi.minor > 0 ) {
							mVersion << ".";
							mVersion << vi.minor;
						}
					}
					switch( vi.variety ) {
						case 0:		mVersion << " Dev";			break;
						case 1:		mVersion << " Alpha";		break;
						case 2:		mVersion << " Beta";		break;
						case 3:		mVersion << " Gamma";		break;
						case 4:		mVersion << " GM";			break;
					}
					if( vi.internal > 0 ) {
						mVersion << " / ";
						mVersion << vi.internal;
					}
				} else {
					mVersion = "Unknown";
				}
			}
		}
		
		mArpPic = BTranslationUtils::GetBitmap('jpeg', 1);
		
		mDrawMode = 0xffffffff;
		mBaseFont = *be_bold_font;
		#if 0
		font_family family;
		strcpy(family, "Baskerville Roman");
		mBaseFont.SetFamilyAndFace(family, B_REGULAR_FACE);
		#endif
		mBaseFont.SetSize(14);
		mBaseFont.SetSpacing(B_CHAR_SPACING);
		mBaseFont.SetFlags(0x002|0x100);
		mOutliers = 20;
		mFlatPart = .1;
		mStep = -mOutliers;
		
		ComputeDimens();
		
		mWebBut = new BButton(BRect(0,0,100,20), "web",
							  "Go Web",
							  new BMessage(GO_WEB_MSG),
							  B_FOLLOW_BOTTOM | B_FOLLOW_LEFT);
		float webW=0, webH=0;
		mWebBut->GetPreferredSize(&webW, &webH);
		mWebBut->ResizeToPreferred();
		mWebBut->MoveTo(5, Bounds().bottom-webH-5);
		AddChild(mWebBut);
		
		mDoneBut = new BButton(BRect(0,0,100,20), "done",
							  "Go Away",
							  new BMessage(B_QUIT_REQUESTED),
							  B_FOLLOW_BOTTOM | B_FOLLOW_LEFT);
		float doneW=0, doneH=0;
		mDoneBut->GetPreferredSize(&doneW, &doneH);
		mDoneBut->ResizeToPreferred();
		mDoneBut->MoveTo(5 + webW + 5, Bounds().bottom-webH-5);
		AddChild(mDoneBut);
		
		mAnimThread = spawn_thread(animEntry, "ARP™ Animator",
									B_NORMAL_PRIORITY, this);
	} catch(...) {
		FreeMemory();
		throw;
	}
}

ArpAboutView::~ArpAboutView()
{
	FreeMemory();
}

void ArpAboutView::FreeMemory()
{
	if( mAnimThread >= 0 ) {
		status_t ret;
		kill_thread(mAnimThread);
		wait_for_thread(mAnimThread,&ret);
		mAnimThread = -1;
	}
	delete mAppPic;
	delete mArpPic;
	delete mBitmap;
	delete mText;
}

void ArpAboutView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	BView* par = Parent();
	if( par ) SetViewColor(par->ViewColor());
	else SetViewColor(0, 0, 0);
	Window()->SetDefaultButton(mDoneBut);
	mWebBut->SetTarget(this);
	
	#if 1
	// Pre-load all the font sizes we will use.
	char preload[129];
	int j=0;
	for( int i=0; i<128; i++ ) {
		if( mUsedChars[i] ) preload[j++] = (char)i;
	}
	preload[j] = 0;
	BFont loader(&mBaseFont);
	BFont prevFont;
	GetFont(&prevFont);
	for( float i=1; i<=mBaseFont.Size(); i++ ) {
		loader.SetSize(i);
		SetFont(&loader);
		DrawString(&preload[0], BPoint(Bounds().right,Bounds().bottom));
		#if 0
				   "abcdefghijklmnopqrstuvwxyz"
				   "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
				   BPoint(Bounds().right,Bounds().bottom));
		#endif
	}
	SetFont(&prevFont);
	#endif
	
	mAnimBaseTime = system_time();
	if( mAnimThread >= 0 ) {
		resume_thread(mAnimThread);
	}
}

void ArpAboutView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	if( mAnimThread >= 0 ) {
		suspend_thread(mAnimThread);
	}
}

void ArpAboutView::GetPreferredSize(float* width, float* height)
{
	if( width ) *width = 500;
	if( height ) *height = 300;
}

void ArpAboutView::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	switch( message->what ) {
		case GO_WEB_MSG: {
			char* argv[2];
			argv[0] = "http://www.angryredplanet.com/beos/ArpTelnet/";
			argv[1] = 0;
			be_roster->Launch("application/x-vnd.Be.URL.http", 1, argv);
		} break;
		default:
			inherited::MessageReceived(message);
	}
}

float ArpAboutView::FindMaxWidth(const BFont& font)
{
	float val = 0;
	size_t i;
	for( i=0; i<mText->size(); i++ ) {
		float w = font.StringWidth(mText->at(i).String());
		if( w > val ) val = w;
	}
	return val;
}

void ArpAboutView::ComputeDimens()
{
	mBaseSize = mBaseFont.Size();
	mRotPart = 1.0-mFlatPart;
	mFlatPos = mFlatPart*mOutliers;
	mFlatPixel = mFlatPos*mBaseSize;
	mTextCenter.x = mArpPic ? (mArpPic->Bounds().right+10) : 30;
	mTextCenter.y = Bounds().top + (Bounds().bottom-Bounds().top)/2;
	mYRot = (mArpPic
				? (mArpPic->Bounds().bottom/2)
				: (Bounds().bottom/2)
			) - (mFlatPixel/2);
	if( mYRot < mFlatPixel*4 ) mYRot = mFlatPixel*4;
	
	ArpD(cdb << ADH << "ComputeDimens():" << endl
					<< "  mBaseSize=" << mBaseSize
					<< ", mFlatPart=" << mFlatPart
					<< ", mRotPart=" << mRotPart << endl
					<< "  mFlatPos=" << mFlatPos
					<< ", mFlatPixel=" << mFlatPixel << endl
					<< "  mTextCenter=" << mTextCenter
					<< ", mYRot=" << mYRot << endl);
}

void ArpAboutView::FrameResized(float new_width, float new_height)
{
	ComputeDimens();
}

#if 0
void ArpAboutView::DrawLine(const char* text, float pos, float center, BFont& font)
{
	if( !mDrawView ) return;
	
	const float ABSPOS = fabs(pos);
	const float ABSREL = ABSPOS/OUTLIERS;
	const float SIGN = pos < 0 ? -1 : 1;
	const float TOTDIST = (OUTLIERS-OUTLIERS/2+OUTLIERS/4) * FONTSIZE;
	
	font.SetShear( 90 - 80.0*sin(pos/OUTLIERS*M_PI_2) );
	//font.SetRotation( -90.0*ABSREL*SIGN );
	font.SetSize( FONTSIZE - ( (FONTSIZE-4)*ABSREL ) );
	mDrawView->SetFont(&font,B_FONT_SIZE|B_FONT_SHEAR);
	mDrawView->SetHighColor(255,255-int(255*ABSREL),0);
	const float STRWID = font.StringWidth(text);
	const float HALFX = mDrawView->Bounds().right/2;
	BPoint point;
	point.x = HALFX - (STRWID/2) + sin(pos/OUTLIERS*M_PI_2)*HALFX;
	//const float yoff = -TOTDIST + cos(ABSPOS/OUTLIERS)*TOTDIST;
	const float yoff = (ABSPOS-ABSPOS/2+ABSPOS/4)*(FONTSIZE) * SIGN;
	point.y = center + yoff;
	mDrawView->DrawString(text, point);
}
#endif

#if 0
void ArpAboutView::DrawLine(const char* text, float pos, float center, BFont& font)
{
	if( !mDrawView ) return;
	
	if( pos < -OUTLIERS || pos > OUTLIERS ) return;
	//if( pos < -OUTLIERS ) pos = -OUTLIERS;
	//if( pos > OUTLIERS ) pos = OUTLIERS;
	
	const float ABSPOS = fabs(pos);
	const float ABSREL = ABSPOS/OUTLIERS;
	const float SIGN = pos < 0 ? -1 : 1;
	//const float TOTDIST = (OUTLIERS-OUTLIERS/2+OUTLIERS/4) * FONTSIZE;
	const float XOFF = mArpPic ? mArpPic->Bounds().right : 30;
	#if 1
	const float YOFF = (mArpPic
							? (mArpPic->Bounds().bottom/2)
							: (Bounds().bottom/2)
						) - (.2*OUTLIERS);
	#else
	const float YOFF = Bounds().bottom - (.2*OUTLIERS);
	#endif
	
	//font.SetRotation( -90.0*sin(pos/OUTLIERS*M_PI_2) );
	BPoint point;
	const float OFFREL = ABSREL > .2 ? ((ABSREL-.2)/.8) : 0;
	//font.SetRotation( -90.0*(1-sqrt(1-OFFREL))*SIGN );
	//font.SetSize( floor( (FONTSIZE - ( (FONTSIZE-4)*OFFREL) ) * 400 ) / 400 );
	#if 1
	if( ABSREL > .2 ) {
		//font.SetRotation( -90.0*(1-sqrt(1-OFFREL))*SIGN );
		font.SetRotation( -90*(ABSREL-.2)/.8*SIGN );
		font.SetSize( FONTSIZE - ( (FONTSIZE-2)*OFFREL ) );
		const float SIGNOFF = .2*SIGN*OUTLIERS;
		point.x = -10 + cos((pos-SIGNOFF)/(OUTLIERS-.2*OUTLIERS)*M_PI_2)*(XOFF+20);
		const float OFFPOS = .2*OUTLIERS;
		//const float yoff = ( OFFPOS*FONTSIZE + ((ABSPOS-OFFPOS)-(ABSPOS-OFFPOS)/2
		//								+(ABSPOS-OFFPOS)/4)*(FONTSIZE) ) * SIGN;
		const float yoff = (OFFPOS*FONTSIZE*SIGN)
							+ sin((pos-SIGNOFF)/(OUTLIERS-.2*OUTLIERS)*M_PI_2)*YOFF;
		point.y = center + yoff;
	} else {
		font.SetRotation( 0 );
		font.SetSize( FONTSIZE );
		//const float SIGNOFF = .2*SIGN;
		//point.x = -10 + cos((.2*SIGN-SIGNOFF)/(OUTLIERS-SIGNOFF)*M_PI_2)*(XOFF+20);
		point.x = -10 + (XOFF+20);
		const float yoff = ABSPOS*FONTSIZE*SIGN;
		point.y = center + yoff;
	}
	#endif
	point.x = -10 + cos(pos/OUTLIERS*M_PI_2)*(XOFF+20);
	//font.SetRotation(0);
	//point.x = XOFF;
	//font.SetRotation( -90.0*ABSREL*SIGN );
	mDrawView->SetFont(&font,B_FONT_SIZE|B_FONT_ROTATION);
	mDrawView->SetHighColor(255,255-int(255*ABSREL),0);
	//point.x = -10 + cos(pos/OUTLIERS*M_PI_2)*(XOFF+20);
	//const float yoff = -TOTDIST + cos(ABSPOS/OUTLIERS)*TOTDIST;
	//const float yoff = (ABSPOS-sin(ABSREL*M_PI_4)*ABSPOS)*(FONTSIZE) * SIGN;
	//const float yoff = (ABSPOS-ABSPOS/3+ABSPOS/4)*(FONTSIZE) * SIGN;
	//point.y = center + yoff;
	if( mDrawMode&2 ) {
		float w = font.StringWidth(text);
		BPoint endp(point);
		endp.x += w*cos(font.Rotation()/180.0*M_PI);
		endp.y -= w*sin(font.Rotation()/180.0*M_PI);
		mDrawView->StrokeLine(point, endp);
	}
	if( mDrawMode&1 ) {
		mDrawView->DrawString(text, point);
	}
	if( 0 && text == mText->at(0).String() ) {
		printf("Pos %f: Rotation=%f, Size=%f, pnt=(%f, %f)\n",
				pos, font.Rotation(), font.Size(), point.x, point.y);
	}
}
#endif

#if 1
void ArpAboutView::DrawLine(const char* text, float pos, float center, BFont& font)
{
	if( !mDrawView ) return;
	
	if( pos < -mOutliers || pos > mOutliers ) return;
	
	const float ABSREL = fabs(pos)/mOutliers;
	const float SIGN = pos < 0 ? -1 : 1;
	const float ROTREL = ABSREL > mFlatPart
					? ((ABSREL-mFlatPart)/mRotPart) : 0;

	BPoint point(mTextCenter);
	//font.SetRotation( -90*ROTREL*SIGN );
	//font.SetShear( 90 + 45*ROTREL*SIGN );
	font.SetRotation(0);
	font.SetSize( mBaseSize - ( (mBaseSize-1)*ROTREL ) );
	//point.x = -10 + cos(pos/mOutliers*M_PI_2)*(point.x+10);
	point.x = Bounds().right - (-10 + cos(pos/mOutliers*M_PI_2)*(point.x+10));
	if( ABSREL > mFlatPart ) {
		point.y += mFlatPixel*SIGN
					+ sin((pos-(mFlatPos*SIGN))/(mOutliers-mFlatPos)*M_PI_2)*mYRot;
	} else {
		point.y += pos*mBaseSize;
	}
	
	mDrawView->SetFont(&font,B_FONT_SIZE|B_FONT_ROTATION|B_FONT_SHEAR);
	if( ABSREL < .5 ) {
		mDrawView->SetHighColor(255,255-int(255*ABSREL/.5),0);
	} else {
		mDrawView->SetHighColor(255-int(255*(ABSREL-.5)),0,0);
	}
	
	float w = font.StringWidth(text);
	mDrawMode = 1;
	if( mDrawMode&2 ) {
		BPoint endp(point);
		endp.x += w*cos(font.Rotation()/180.0*M_PI);
		endp.y -= w*sin(font.Rotation()/180.0*M_PI);
		mDrawView->StrokeLine(point, endp);
	}
	if( mDrawMode&1 ) {
		point.x -= w;
		mDrawView->DrawString(text, point);
	}
	if( text == mText->at(0).String() ) {
		ArpDL(__FILE__, 2,
			 cdb << ADH << "Pos " << pos << ": Rotation=" << font.Rotation()
						<< ", Size=" << font.Size()
						<< ", Pnt=" << point << endl);
	}
}
#endif

void ArpAboutView::Draw(BRect)
{
	// Set up the backing store bitmap.
	{
		BScreen screen(Window());
		if( !mBitmap || mBitmap->Bounds() != Bounds()
			|| mBitmap->ColorSpace() != screen.ColorSpace() ) {
			Sync();
			delete mBitmap;
			mBitmap = 0;
			mDrawView = 0;
			mBitmap = new BBitmap(Bounds(), screen.ColorSpace(), true);
			mDrawView = new BView(Bounds(), "BackingStore",
								  B_FOLLOW_ALL, B_WILL_DRAW);
			mBitmap->AddChild(mDrawView);
		}
	}
	
	if( !mBitmap || !mDrawView ) return;
	if( !mBitmap->Lock() ) return;
	
	int i;
	const int CURLINE = int(mStep);
	
	BRect bounds = mDrawView->Bounds();
	
	mDrawView->SetLowColor(0,0,0);
	mDrawView->SetHighColor(0,0,0);
	
#if 0
	printf("Step: %f, Bounds: (%f,%f)-(%f,%f)\n", mStep,
			bounds.left, bounds.top, bounds.right, bounds.bottom);
#endif

	mDrawView->FillRect(bounds);
	
	float showAmount = (mStep+mOutliers)/mOutliers;
	if( showAmount > 1.0 || !mFirstTime ) {
		showAmount = 1.0;
		mFirstTime = false;
	}
	float barAmount = showAmount*3;
	if( barAmount > 1.0 ) barAmount = 1.0;
	
	BFont curFont(&mBaseFont);
	curFont.SetSpacing(B_BITMAP_SPACING);
	curFont.SetSize(32);
	//curFont.SetFace(B_BOLD_FACE);
	font_height height;
	curFont.GetHeight(&height);
	float swidth = curFont.StringWidth(mAppName.String());
	
	float boxLeft = -100;
	float boxRight = 6 + (mAppPic?mAppPic->Bounds().right:0)
					+ 5 + swidth + 30;
	boxLeft += int( (bounds.Width()+100)*(1-barAmount) );
	boxRight += int( (bounds.Width()+100)*(1-barAmount) );
	if( boxLeft < 0 ) boxLeft = 0;
	if( boxRight > bounds.Width() ) boxRight = bounds.Width();
	
	float boxTop = 3;
	float boxBottom = height.ascent+height.descent;
	const float origBottom = boxBottom;
	float pich = boxBottom;
	if( mAppPic && boxBottom < mAppPic->Bounds().bottom ) {
		boxBottom = mAppPic->Bounds().bottom;
	}
	boxBottom += 6;
	
	const int Ngrad = int(boxBottom-boxTop)/2 - 4;
	for( i=0; i<Ngrad; i++ ) {
		mDrawView->SetHighColor(0,0,(128*i)/Ngrad);
		mDrawView->StrokeLine(BPoint(boxLeft,boxTop),
							  BPoint(boxRight,boxTop));
		mDrawView->StrokeLine(BPoint(boxLeft,boxBottom),
							  BPoint(boxRight,boxBottom));
		boxTop+=1;
		boxBottom-=1;
	}
	mDrawView->SetHighColor(0,0,128);
	mDrawView->FillRect(BRect(boxLeft,boxTop,boxRight,boxBottom));
	
	BPoint pos(6,6);
	if( mAppPic ) {
		mDrawView->SetDrawingMode(B_OP_OVER);
		mDrawView->DrawBitmapAsync(mAppPic, pos);
		mDrawView->SetDrawingMode(B_OP_COPY);
		pos.x += mAppPic->Bounds().right + 5;
		pich = mAppPic->Bounds().bottom;
	}
	
	//pos.y += height.ascent;
	pos.y += height.ascent + (pich-height.ascent-height.descent)/2;
	mDrawView->SetFont(&curFont);
	mDrawView->SetLowColor(B_TRANSPARENT_COLOR);
	mDrawView->SetHighColor(255,255,255);
	mDrawView->DrawString(mAppName.String(), pos);
	
	if( mVersion != "" ) {
		BPoint vp(pos);
		vp.y = origBottom + 6;
		BFont verFont(be_plain_font);
		verFont.SetSize(16);
		//verFont.SetFace(B_REGULAR_FACE);
		font_height height;
		verFont.GetHeight(&height);
		mDrawView->SetFont(&verFont);
		vp.y += height.ascent;
		mDrawView->SetLowColor(0,0,0);
		mDrawView->SetHighColor(int(barAmount*255),
								int(barAmount*255),
								int(barAmount*255));
		mDrawView->DrawString("Version ", vp);
		mDrawView->DrawString(mVersion.String());
	}
	
	if( boxLeft > 0 ) {
		mDrawView->SetHighColor(0,0,0);
		mDrawView->FillRect(BRect(0,0,boxLeft-1,origBottom+6));
	}
	
	if( mArpPic ) {
		//pos.x = bounds.right-mArpPic->Bounds().right;
		//pos.y = 0;
		pos.x = bounds.right-mArpPic->Bounds().right-5;
		pos.y = (bounds.bottom/2)-(mArpPic->Bounds().bottom/2);
		pos.x += (bounds.right-pos.x+1)*(1.0-showAmount);
		mDrawView->DrawBitmapAsync(mArpPic, pos);
#if 0
		if( showAmount >= 1.0 ) {
			mDrawView->DrawBitmapAsync(mArpPic, pos);
		} else {
			BRect size;
			const float off = (1.0-showAmount)/2;
			const float w = mArpPic->Bounds().right;
			const float h = mArpPic->Bounds().bottom;
			size.left = pos.x; // + (w*off);
			size.right = pos.x + w - (w*off*2);
			size.top = pos.y + (h*off);
			size.bottom = pos.y + h - (h*off);
			mDrawView->DrawBitmapAsync(mArpPic,size);
		}
#endif
	}
	
	curFont.SetSpacing(B_CHAR_SPACING);
	//curFont.SetFace(B_BOLD_FACE);
	mDrawView->SetFont(&curFont);
	
	mDrawView->SetLowColor(0,0,0);
	for( i=-mOutliers; i<=mOutliers; i++ ) {
		const size_t line = CURLINE+i;
		if( line >= 0 && line < N ) {
			DrawLine(mText->at(line).String(), (int(mStep)-mStep+i),
					 bounds.top+ (bounds.bottom-bounds.top)/2, curFont);
		}
	}
	
	mDrawView->Sync();
	DrawBitmapAsync(mBitmap, BPoint(0,0));
	mBitmap->Unlock();
}

void ArpAboutView::MouseDown(BPoint)
{
	mDrawMode++;
}

void ArpAboutView::StepAnim()
{
	mStep = float(system_time()-mAnimBaseTime)/700000 - mOutliers;
	if( mStep > (N+mOutliers) ) mAnimBaseTime = system_time();
	Draw(Bounds());
}

int32 ArpAboutView::animEntry(void *arg) 
{ 
	ArpAboutView *obj = (ArpAboutView *)arg; 
	return (obj->animFunc()); 
}

int32 ArpAboutView::animFunc()
{
	while( 1 ) {
		bigtime_t nexttime = system_time() + 50000;
		if( LockLooper() ) {
			StepAnim();
			UnlockLooper();
		}
		if( nexttime < system_time() ) nexttime = system_time() + 1000;
		snooze_until(nexttime, B_SYSTEM_TIMEBASE);
	}
}
