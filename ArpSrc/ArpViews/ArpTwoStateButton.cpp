/* ArpTwoStateButton.cpp
 */
#include <cstdio>
#include <interface/Bitmap.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViews/ArpTwoStateButton.h"

/***************************************************************************
 * ARP-TWO-STATE-BUTTON
 ***************************************************************************/
ArpTwoStateButton::ArpTwoStateButton(	BRect frame, const char* name,
										const char* label,
										BMessage* message,
										const BBitmap* bmNormal,
										const BBitmap* bmOver,
										const BBitmap* bmPressed,
										const BBitmap* bmDisabled,
										const BBitmap* bmDisabledPressed,
										uint32 resizeMask)
		: inherited(frame, name, label, message, resizeMask, B_WILL_DRAW | B_NAVIGABLE),
		  mBmNormal(bmNormal), mBmPressed(bmPressed),
		  mPressed(false), mSwitched(false), mDrawFromSwitched(false)
{
}

void ArpTwoStateButton::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( Parent() ) SetViewColor( Parent()->ViewColor() );
}

void ArpTwoStateButton::Draw(BRect clip)
{
	inherited::Draw(clip);
	drawing_mode	mode = DrawingMode();
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	bool	state = (mDrawFromSwitched) ? mSwitched : mPressed;
	if( state && mBmPressed ) {
		BRect	r( clip );
		BRect	b( mBmPressed->Bounds() );
		if( r.right > b.right ) r.right = b.right;
		if( r.bottom > b.bottom ) r.bottom = b.bottom;
		DrawBitmapAsync( mBmPressed, r, r );
	} else if( !state && mBmNormal ) {
		BRect	r( clip );
		BRect	b( mBmNormal->Bounds() );
		if( r.right > b.right ) r.right = b.right;
		if( r.bottom > b.bottom ) r.bottom = b.bottom;
		DrawBitmapAsync( mBmNormal, r, r );
	}
	if (Window()) Sync();
	SetDrawingMode(mode);
}

void ArpTwoStateButton::MouseDown(BPoint where)
{
	inherited::MouseDown(where);
	mSwitched = !mPressed;

	/* Quick hack */
	mPressed = !mPressed;
	BMessage*	msg = 0;
	if( Message() ) msg = new BMessage( *Message() );
	if( msg ) msg->AddBool("on", mPressed);
	Invoke(msg);

	Invalidate();
}

void ArpTwoStateButton::MouseMoved(	BPoint where,
									uint32 code,
									const BMessage* message)
{
	inherited::MouseMoved(where, code, message);
}

void ArpTwoStateButton::MouseUp(BPoint where)
{
	inherited::MouseUp(where);
}

void ArpTwoStateButton::SetButtonState(bool pressed)
{
	mPressed = pressed;
	BMessage*	msg = 0;
	if( Message() ) msg = new BMessage( *Message() );
	if( msg ) msg->AddBool("on", mPressed);
	Invoke(msg);

	Invalidate();
}

bool ArpTwoStateButton::ButtonState() const
{
	return mPressed;
}