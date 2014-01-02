/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BitmapButton.h"

#include <ControlLook.h>
#include <LayoutUtils.h>
#include <Window.h>


BBitmapButton::BBitmapButton(const char* name, BMessage* message)
	:
	BitmapView(name),
	BInvoker(message, NULL, NULL),
	fMouseInside(false),
	fMouseDown(false)
{
	SetScaleBitmap(false);
}


BBitmapButton::~BBitmapButton()
{
}


void
BBitmapButton::AttachedToWindow()
{
	// TODO: Init fMouseInside
	BitmapView::AttachedToWindow();
}

void
BBitmapButton::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	int32 buttons = 0;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	bool ignoreEvent = dragMessage != NULL || (!fMouseDown && buttons != 0);

	_SetMouseInside(!ignoreEvent && transit == B_INSIDE_VIEW);
}


void
BBitmapButton::MouseDown(BPoint where)
{
	if (fMouseInside) {
		_SetMouseDown(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}


void
BBitmapButton::MouseUp(BPoint where)
{
	_SetMouseDown(false);
	if (fMouseInside)
		Invoke();
}


BSize
BBitmapButton::MinSize()
{
	BSize size = BitmapView::MinSize();
	size.width += 6;
	size.height += 6;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BBitmapButton::PreferredSize()
{
	BSize size = MinSize();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


BSize
BBitmapButton::MaxSize()
{
	BSize size = MinSize();
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


// #pragma mark -


void
BBitmapButton::DrawBackground(BRect& bounds, BRect updateRect)
{
	if (Message() != NULL && (fMouseInside || fMouseDown)) {
		rgb_color color = LowColor();
		uint32 flags = 0;
		if (fMouseDown)
			flags |= BControlLook::B_ACTIVATED;

		be_control_look->DrawButtonFrame(this, bounds, updateRect,
			color, color, flags);
		be_control_look->DrawButtonBackground(this, bounds, updateRect,
			color, flags);
	} else {
		BitmapView::DrawBackground(bounds, updateRect);
	}
}


// #pragma mark -


void
BBitmapButton::_SetMouseInside(bool inside)
{
	if (fMouseInside == inside)
		return;

	fMouseInside = inside;

	if (Message() != NULL)
		Invalidate();
}


void
BBitmapButton::_SetMouseDown(bool down)
{
	if (fMouseDown == down)
		return;

	fMouseDown = down;

	if (Message() != NULL)
		Invalidate();
}
