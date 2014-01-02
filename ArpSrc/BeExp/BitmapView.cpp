/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "BitmapView.h"

#include <algorithm>

#include <Bitmap.h>
#include <LayoutUtils.h>


BitmapView::BitmapView(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fBitmap(NULL),
	fScaleBitmap(true)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BitmapView::~BitmapView()
{
}


void
BitmapView::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent != NULL) {
		rgb_color color = parent->ViewColor();
		if (color == B_TRANSPARENT_COLOR)
			color = parent->LowColor();
		SetLowColor(color);
	}
}


void
BitmapView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	DrawBackground(bounds, updateRect);

	if (fBitmap == NULL)
		return;

	BRect bitmapBounds = fBitmap->Bounds();
	if (bitmapBounds.Width() <= 0.0f || bitmapBounds.Height() <= 0.0f)
		return;

	float scale = 1.0f;

	if (fScaleBitmap) {
		float hScale = bounds.Width() / bitmapBounds.Width();
		float vScale = bounds.Height() / bitmapBounds.Height();

		scale = std::min(hScale, vScale);
	}

	float width = bitmapBounds.Width() * scale;
	float height = bitmapBounds.Height() * scale;

	switch (LayoutAlignment().horizontal) {
		case B_ALIGN_LEFT:
			break;
		case B_ALIGN_RIGHT:
			bounds.left = floorf(bounds.right - width);
			break;
		default:
		case B_ALIGN_HORIZONTAL_CENTER:
			bounds.left = floorf(bounds.left
				+ (bounds.Width() - width) / 2.0f);
			break;
	}
	switch (LayoutAlignment().vertical) {
		case B_ALIGN_TOP:
			break;
		case B_ALIGN_BOTTOM:
			bounds.top = floorf(bounds.bottom - height);
			break;
		default:
		case B_ALIGN_VERTICAL_CENTER:
			bounds.top = floorf(bounds.top
				+ (bounds.Height() - height) / 2.0f);
			break;
	}

	bounds.right = ceilf(bounds.left + width);
	bounds.bottom = ceilf(bounds.top + height);

	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap, bitmapBounds, bounds, B_FILTER_BITMAP_BILINEAR);
}


BSize
BitmapView::MinSize()
{
	BSize size(0.0f, 0.0f);

	if (fBitmap != NULL) {
		BRect bounds = fBitmap->Bounds();
		size.width = bounds.Width();
		size.height = bounds.Height();
	}

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
BitmapView::PreferredSize()
{
	BSize size = MinSize();
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


BSize
BitmapView::MaxSize()
{
	BSize size = MinSize();
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


void
BitmapView::SetBitmap(const BBitmap* bitmap)
{
	if (bitmap == fBitmap)
		return;

	BSize size = MinSize();

	fBitmap = bitmap;

	BSize newSize = MinSize();
	if (size != newSize)
		InvalidateLayout();

	Invalidate();
}


void
BitmapView::SetScaleBitmap(bool scaleBitmap)
{
	if (scaleBitmap == fScaleBitmap)
		return;

	fScaleBitmap = scaleBitmap;

	Invalidate();
}


void
BitmapView::DrawBackground(BRect& bounds, BRect updateRect)
{
	FillRect(updateRect, B_SOLID_LOW);
}
