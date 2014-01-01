/* ArpIntControlMotions.cpp
 */
#include <stdio.h>
#include <string.h>
#include <be/app/Message.h>
#include <be/interface/Menu.h>
#include <be/interface/MenuItem.h>
#include <be/interface/View.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"

/* Hmmm.  Perhaps clients would like to be able to set this?
 */
static const float ARP_SMALL_STEP		= 10;

void arp_setup_scroll_bar(BScrollBar& bar, BRect targetBounds, float targetSize)
{
	/* Set the range -- HMM, this looks awfully orientation-centric.
	 */
	{
		float		width = targetSize;
		if( targetSize <= targetBounds.Width() ) width = 0;
		bar.SetRange( 0, width );
	}

	float	min, max;
	bar.GetRange(&min, &max);
	if( min != 0 || max != 0 ) {
		/* Set the steps
		 */
		float		bigStep = 0, smallStep = ARP_SMALL_STEP;
		if( bar.Orientation() == B_HORIZONTAL ) bigStep = targetBounds.Width();
		if( bar.Orientation() == B_VERTICAL ) bigStep = targetBounds.Height();

		if( bigStep > 20 ) bigStep -= smallStep;
		if( bigStep > targetSize - targetBounds.Width() ) bigStep = targetSize - targetBounds.Width();
		if( (targetBounds.right + smallStep) > targetSize ) smallStep = targetSize - targetBounds.right;
		bar.SetSteps(smallStep, bigStep);
		/* Set the proportion
		 */		
		float	prop = targetBounds.Width() / max;
		if( prop > 1 ) prop = 1;
		bar.SetProportion(prop);
	}
}

void arp_setup_horizontal_scroll_bar(BScrollBar* bar, BView* preferredTarget)
{
	ArpASSERT(bar && preferredTarget);
	float				preferredWidth, preferredHeight, proportion;
	preferredTarget->GetPreferredSize(&preferredWidth, &preferredHeight);
	proportion = preferredTarget->Bounds().Width() / preferredWidth;
	if (proportion < 0) proportion = 0;
	if (proportion > 1) proportion = 1;
	bar->SetProportion(proportion);
	float			width = preferredWidth - preferredTarget->Frame().Width();
	if (width < 0) width = 0;
	bar->SetRange(0, width);
}

void arp_setup_vertical_scroll_bar(BScrollBar* bar, BView* preferredTarget)
{
	ArpASSERT(bar && preferredTarget);
	float				preferredWidth, preferredHeight, proportion;
	preferredTarget->GetPreferredSize(&preferredWidth, &preferredHeight);
	proportion = preferredTarget->Bounds().Height() / preferredHeight;
	if (proportion < 0) proportion = 0;
	if (proportion > 1) proportion = 1;
	bar->SetProportion(proportion);
	float			height = preferredHeight - preferredTarget->Frame().Height();
	if (height < 0) height = 0;
	bar->SetRange(0, height);
}

float arp_get_font_height(const BFont* font)
{
	if( !font ) return 0;
	font_height		fh;
	font->GetHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

float arp_get_font_height(const BView* view)
{
	if( !view ) return 0;
	BFont	font;
	view->GetFont( &font );
	return arp_get_font_height( &font );
}

BMenuItem* add_menu_item(	BMenu* toMenu,
							const char* label,
							BMessage* msg,
							char shortcut,
							uint32 modifiers)
{
	BMenuItem	*item = new BMenuItem(label, msg, shortcut, modifiers);
	if (!item) {
		delete msg;
		return NULL;
	}
	toMenu->AddItem(item);
	return item;
}

BMenuItem* add_menu_item(	BMenu* toMenu,
							const char* label,
							uint32 what,
							char shortcut,
							uint32 modifiers)
{
	BMessage	*msg = new BMessage(what);
	if (!msg) return NULL;
	return add_menu_item(toMenu, label, msg, shortcut, modifiers);
}

void arp_mark_item(const char* label, BMenu* menu)
{
	if (!label || !menu) return;
	BMenuItem*	item;
	for (int32 k = 0; (item = menu->ItemAt(k)) != NULL; k++) {
		if (strcmp(item->Label(), label) == 0) {
			item->SetMarked(true);
		}
	}
}

void arp_tile_bitmap_on(BView* view, BRect clip,
						const BBitmap* bm, BPoint offset)
{
	ArpASSERT(view && bm);
	BRect		origDest(bm->Bounds());
	float		w = origDest.Width();
	float		h = origDest.Height();
	while ( (origDest.right + w) < clip.left) origDest.OffsetBy(w + 1, 0);
	while ( (origDest.bottom + h) < clip.top) origDest.OffsetBy(0, h + 1);
	
	BRect		dest(origDest);
	dest.OffsetBy(-offset.x, -offset.y);
	while (dest.left <= clip.right && dest.top <= clip.bottom) {
		view->DrawBitmapAsync(bm, dest);
		dest.OffsetBy(w + 1, 0);
		if (dest.left > clip.right)
			dest.OffsetTo(origDest.left, dest.bottom + 1);
	}
}

BRect arp_invalid_rect()
{
	return BRect(-1, -1, -1, -1);
}

bool arp_is_valid_rect(const BRect& r)
{
	return r.left != -1 && r.top != -1 && r.right != -1 && r.bottom != -1;
}

BRect arp_merge_rects(const BRect& r1, const BRect& r2)
{
	if (!arp_is_valid_rect(r1)) return r2;
	else if (!arp_is_valid_rect(r2)) return r1;
	else return r1 | r2;
}
