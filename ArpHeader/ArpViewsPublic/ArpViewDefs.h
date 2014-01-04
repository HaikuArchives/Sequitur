/* ArpViewDefs.h
 * Global definitions for Arp view classes.
 */

#ifndef ARPVIEWSPUBLIC_ARPVIEWDEFS_H
#define ARPVIEWSPUBLIC_ARPVIEWDEFS_H

#include <interface/Bitmap.h>
#include <interface/ScrollBar.h>
#include <interface/View.h>
#include <support/SupportDefs.h>

class BMenu;
class BMenuItem;

/* Various objects in the system can be referenced by their ID's.  This
 * is particularly useful when passing references around in views.
 */
typedef void*			view_id;
#define SZ_VIEW_ID		"view_id"

/*----------------------------------------------------------------*/

	/*---------------------------------------------------------
	 * IMAGE CONSTANTS.  Use the ImageManager() to access the
	 * actual images.
	 *---------------------------------------------------------*/

#define MINI_DIGIT_0_STR			"Mini Digit 0"
#define MINI_DIGIT_1_STR			"Mini Digit 1"
#define MINI_DIGIT_2_STR			"Mini Digit 2"
#define MINI_DIGIT_3_STR			"Mini Digit 3"
#define MINI_DIGIT_4_STR			"Mini Digit 4"
#define MINI_DIGIT_5_STR			"Mini Digit 5"
#define MINI_DIGIT_6_STR			"Mini Digit 6"
#define MINI_DIGIT_7_STR			"Mini Digit 7"
#define MINI_DIGIT_8_STR			"Mini Digit 8"
#define MINI_DIGIT_9_STR			"Mini Digit 9"
#define MAGNIFY_SMALL_IMAGE_STR		"Magnify Small"
#define	KNOB_000_IMAGE_STR			"Knob 000"
#define	KNOB_010_IMAGE_STR			"Knob 010"
#define	KNOB_020_IMAGE_STR			"Knob 020"
#define	KNOB_030_IMAGE_STR			"Knob 030"
#define	KNOB_040_IMAGE_STR			"Knob 040"
#define	KNOB_050_IMAGE_STR			"Knob 050"
#define	KNOB_060_IMAGE_STR			"Knob 060"
#define	KNOB_070_IMAGE_STR			"Knob 070"
#define	KNOB_080_IMAGE_STR			"Knob 080"
#define	KNOB_090_IMAGE_STR			"Knob 090"
#define	KNOB_100_IMAGE_STR			"Knob 100"
#define	KNOB_110_IMAGE_STR			"Knob 110"
#define	KNOB_120_IMAGE_STR			"Knob 120"
#define	KNOB_130_IMAGE_STR			"Knob 130"
#define	KNOB_140_IMAGE_STR			"Knob 140"
#define	KNOB_150_IMAGE_STR			"Knob 150"
#define	KNOB_160_IMAGE_STR			"Knob 160"
#define	KNOB_170_IMAGE_STR			"Knob 170"
#define	KNOB_180_IMAGE_STR			"Knob 180"
#define	KNOB_190_IMAGE_STR			"Knob 190"
#define	KNOB_200_IMAGE_STR			"Knob 200"
#define	KNOB_210_IMAGE_STR			"Knob 210"
#define	KNOB_220_IMAGE_STR			"Knob 220"
#define	KNOB_230_IMAGE_STR			"Knob 230"
#define	KNOB_240_IMAGE_STR			"Knob 240"
#define	KNOB_250_IMAGE_STR			"Knob 250"
#define	KNOB_260_IMAGE_STR			"Knob 260"
#define	KNOB_270_IMAGE_STR			"Knob 270"
#define	KNOB_280_IMAGE_STR			"Knob 280"
#define	KNOB_290_IMAGE_STR			"Knob 290"
#define	KNOB_300_IMAGE_STR			"Knob 300"
#define	KNOB_310_IMAGE_STR			"Knob 310"
#define	KNOB_320_IMAGE_STR			"Knob 320"
#define	KNOB_330_IMAGE_STR			"Knob 330"
#define	KNOB_340_IMAGE_STR			"Knob 340"
#define	KNOB_350_IMAGE_STR			"Knob 350"
#define	KNOB_RING_IMAGE_STR			"Knob Ring"
#define	KNOB_RING_TIGHT_IMAGE_STR	"Knob Ring Tight"

/*----------------------------------------------------------------*/
void		arp_setup_scroll_bar(BScrollBar& bar, BRect targetBounds, float targetSize);
void		arp_setup_horizontal_scroll_bar(BScrollBar* bar, BView* preferredTarget);
void		arp_setup_vertical_scroll_bar(BScrollBar* bar, BView* preferredTarget);
/* Because constantly getting the actual font height (ascent + descent + leading)
 * gets so damn tiresome.
 */
float		arp_get_font_height(const BFont* font);
float		arp_get_font_height(const BView* view);
/* Because adding menu items is pretty damn repetitious.
 */
BMenuItem*	add_menu_item(	BMenu* toMenu, const char* label, BMessage* msg,
							char shortcut, uint32 modifiers = B_COMMAND_KEY);
BMenuItem*	add_menu_item(	BMenu* toMenu, const char* label, uint32 what,
							char shortcut, uint32 modifiers = B_COMMAND_KEY);
/* Function to set the item with the supplied label to marked.
 */
void		arp_mark_item(const char* label, BMenu* menu);
/* Function to tile the supplied bitmap on the view in the clipping
 * bounds.  If the view has a parent whose tile pattern it's trying to
 * match, it can supply an offset.  For example, a view matching its
 * immediate parent would just supply its Frame().LeftTop();
 */
void		arp_tile_bitmap_on(	BView* view, BRect clip,
								const BBitmap* bm, BPoint offset);
/* These three methods are useful for merging a series of BRects.
 * Create an initial rect set to invalid_rect(), then keep merging
 * any necessary rects with it.  If the final answer is_valid(),
 * go ahead and use it.  (This is typically used for building up
 * an area to invalidate)
 */
BRect		arp_invalid_rect();
bool		arp_is_valid_rect(const BRect& r);
BRect		arp_merge_rects(const BRect& r1, const BRect& r2);

#endif
