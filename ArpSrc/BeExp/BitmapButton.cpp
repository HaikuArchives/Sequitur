/*
	Copyright 2000, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <BeBuild.h>

namespace BExperimental {
class _EXPORT BBitmapButton;
}

#include "BitmapButton.h"

#include <Screen.h>
#include <Window.h>

#include <Debug.h>

#include "BitmapTools.h"
#include "ColorTools.h"

#define AUTO_BITMAP_GENERATION 1
#define FANCY_OVER_SHADOWS 1

#if AUTO_BITMAP_GENERATION

// -------------------- recolor_bitmap --------------------

typedef bool (*recolor_bitmap_func)(rgb_color* inout_color, void* data);

static status_t recolor_bitmap(BBitmap* bm, recolor_bitmap_func func, void* data)
{
	rgb_color color;
	
	switch( bm->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		{
			uint8* start = (uint8*)bm->Bits();
			uint8* end = start + bm->BitsLength();
			
			while( start < end ) {
				color.blue = start[0];
				color.green = start[1];
				color.red = start[2];
				color.alpha = start[3];
				if( (*func)(&color, data) ) {
					start[0] = color.blue;
					start[1] = color.green;
					start[2] = color.red;
					start[3] = color.alpha;
				}
				start += 4;
			}
		} break;
		
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		{
			uint8* start = (uint8*)bm->Bits();
			uint8* end = start + bm->BitsLength();
			
			while( start < end ) {
				color.blue = start[3];
				color.green = start[2];
				color.red = start[1];
				color.alpha = start[0];
				if( (*func)(&color, data) ) {
					start[3] = color.blue;
					start[2] = color.green;
					start[1] = color.red;
					start[0] = color.alpha;
				}
				start += 4;
			}
		} break;
		
		case B_RGB16:
		{
			uint16* start = (uint16*)bm->Bits();
			uint16* end = start + bm->BitsLength()/2;
			
			while( start < end ) {
				color.red = (((*start>>11)&0x1f)*0xff)/0x1f;
				color.green = (((*start>>5)&0x3f)*0xff)/0x3f;
				color.blue = ((*start&0x1f)*0xff)/0x1f;
				color.alpha = 255;
				if( (*func)(&color, data) ) {
					*start = (((uint16)color.red & 0xf8) << 8) |
								(((uint16)color.green & 0xfc) << 3) |
								((color.blue & 0xf8) >> 3) ;
				}
				start++;
			}
		} break;
		
		case B_RGB15:
		case B_RGBA15:
		{
			uint16* start = (uint16*)bm->Bits();
			uint16* end = start + bm->BitsLength()/2;
			
			while( start < end ) {
				color.red = (((*start>>10)&0x1f)*0xff)/0x1f;
				color.green = (((*start>>5)&0x1f)*0xff)/0x1f;
				color.blue = ((*start&0x1f)*0xff)/0x1f;
				color.alpha = *start&0x8000 ? 255 : 0;
				if( (*func)(&color, data) ) {
					*start = (((uint16)color.red & 0xf8) << 7) |
								(((uint16)color.green & 0xf8) << 2) |
								((color.blue & 0xf8) >> 3) |
								(color.alpha >= 128 ? 0x8000 : 0) ;
				}
				start++;
			}
		} break;
		
		case B_CMAP8:
		{
			uint8* start = (uint8*)bm->Bits();
			uint8* end = start + bm->BitsLength();
			
			BScreen s;
			const color_map* cm = system_colors();
			
			while( start < end ) {
				if( *start == B_TRANSPARENT_MAGIC_CMAP8 ) {
					color = B_TRANSPARENT_COLOR;
				} else {
					color = cm->color_list[*start];
				}
				if( (*func)(&color, data) ) {
					if( color.alpha < 128 ) {
						*start = B_TRANSPARENT_MAGIC_CMAP8;
					} else {
						*start = cm->index_map[		((color.red&0xF8)<<7)
												|	((color.green&0xF8)<<2)
												|	((color.blue&0xF8)>>3)
											];
					}
				}
				start++;
			}
		} break;
		
		default:
			TRESPASS();
			return B_BAD_VALUE;
	}
	
	return B_OK;
}

// -------------------- disable_bitmap --------------------

static bool disable_bitmap_alpha_func(rgb_color* c, void* background)
{
	(void)background;
	
	if( c->alpha == 0 ) return false;
	
	//uint8 gray = ( c->red/50 + c->green/30 + c->blue/75 ) * 15;
	uint8 gray = ( c->red/3 + c->green/3 + c->blue/3 );
	c->red = c->blue = c->green = gray;
	c->alpha /= 2;
	
	return true;
}

static bool disable_bitmap_color_func(rgb_color* c, void* background)
{
	if( c->alpha == 0 ) return false;
	
	uint8 gray = c->red/6 + c->green/6 + c->blue/6;
	c->red = gray + ((rgb_color*)background)->red/2;
	c->green = gray + ((rgb_color*)background)->green/2;
	c->blue = gray + ((rgb_color*)background)->blue/2;
	
	return true;
}

static status_t disable_bitmap(BBitmap* bm, rgb_color background)
{
	if( background.alpha == 0 ) {
		return recolor_bitmap(bm, disable_bitmap_alpha_func, &background);
	}
	
	return recolor_bitmap(bm, disable_bitmap_color_func, &background);
}

// -------------------- tint_bitmap --------------------

static bool tint_bitmap_func(rgb_color* c, void* background)
{
	if( c->alpha == 0 ) return false;
	
	const uint8 mix = ((rgb_color*)background)->alpha;
	c->red = (uint8)( ( ((uint16)c->red)*(255-mix)
						+ ((uint16)((rgb_color*)background)->red)*(mix)
						) / 255 );
	c->green = (uint8)( ( ((uint16)c->green)*(255-mix)
						+ ((uint16)((rgb_color*)background)->green)*(mix)
						) / 255 );
	c->blue = (uint8)( ( ((uint16)c->blue)*(255-mix)
						+ ((uint16)((rgb_color*)background)->blue)*(mix)
						) / 255 );
	
	return true;
}

static status_t tint_bitmap(BBitmap* bm, rgb_color color)
{
	return recolor_bitmap(bm, tint_bitmap_func, &color);
}

// -------------------- shadow_bitmap --------------------

enum shadow_op {
	B_SHADOW_END = 0,
	B_SHADOW_EXTERIOR,
	B_SHADOW_INTERIOR
};

struct shadow_offset {
	shadow_op op;
	rgb_color color;
	int32 x_offset;
	int32 y_offset;
};

enum shadow_mode {
	B_SHADOW_SPREAD = 0,
	B_SHADOW_SPOT,
	B_SHADOW_X,
	B_SHADOW_Y
};

static status_t shadow_bitmap(BBitmap* bm, const shadow_offset* offsets,
							  const BBitmap* source = 0)
{
	enum {
		L_BLUE,
		L_GREEN,
		L_RED,
		L_ALPHA
	};

	if( bm->ColorSpace() != B_RGB32 && bm->ColorSpace() != B_RGBA32 ) {
		TRESPASS();
		return B_ERROR;
	}
	
	if( !source ) source = bm;
	
	if( bm->ColorSpace() != B_RGB32 && bm->ColorSpace() != B_RGBA32 ) {
		TRESPASS();
		return B_ERROR;
	}
	
	bool all_exterior = true;
	bool all_interior = true;
	const shadow_offset* so = offsets;
	while( so && (all_exterior || all_interior) ) {
		if( so->op == B_SHADOW_INTERIOR ) all_exterior = false;
		else if( so->op == B_SHADOW_EXTERIOR ) all_interior = false;
		else break;
		so++;
	}
	
	const uint8* const sourceMin = (const uint8*)source->Bits();
	const uint8* const sourceMax = sourceMin + source->BitsLength();
	uint8* const destMin = (uint8*)bm->Bits();
	uint8* const destMax = destMin + bm->BitsLength();
	
	const uint8* sourcePos = sourceMin;
	uint8* destPos = destMin;
	const size_t sourceRow = source->BytesPerRow();
	const size_t destRow = bm->BytesPerRow();
	
	while( sourcePos < sourceMax ) {
		const uint8* const sourceEOL = sourcePos + sourceRow;
		uint8* const destEOL = destPos + destRow;
		const uint8* source = sourcePos;
		uint8* dest = destPos;
		for( ; source < sourceEOL; source+=4, dest+=4 ) {
		
			// If all shadows are exterior, source pixel must be
			// somewhat opaque.
			if( all_exterior && source[L_ALPHA] == 0 ) continue;
			
			// If all shadows are interior, source pixel must be
			// somewhat transparent.
			if( all_interior && source[L_ALPHA] == 255 ) continue;
			
			for( so = offsets; so && so->op != B_SHADOW_END; so++ ) {
				uint8* realDest = dest + so->x_offset*4;
				if( realDest < destPos || realDest >= destEOL ) continue;
				realDest += so->y_offset*destRow;
				if( realDest < destMin || realDest >= destMax ) continue;
				
				// If shadow is exterior, destination pixel must be
				// somewhat transparent and source somewhat opaque.
				if( so->op == B_SHADOW_EXTERIOR &&
						(realDest[L_ALPHA] == 255 || source[L_ALPHA] == 0) ) continue;
				
				// If shadow is interior, destination pixel must be
				// somewhat opaque and source somewhat transparent.
				if( so->op == B_SHADOW_INTERIOR &&
						(realDest[L_ALPHA] == 0 || source[L_ALPHA] == 255) ) continue;
				
				const int16 total = realDest[L_ALPHA] + so->color.alpha;
				const uint8 mix = (uint8)( (255*so->color.alpha)/total );
				realDest[L_RED] = (uint8)( ( ((uint16)realDest[L_RED])*(255-mix)
									+ ((uint16)so->color.red)*(mix)
									) / 255 );
				realDest[L_GREEN] = (uint8)( ( ((uint16)realDest[L_GREEN])*(255-mix)
									+ ((uint16)so->color.green)*(mix)
									) / 255 );
				realDest[L_BLUE] = (uint8)( ( ((uint16)realDest[L_BLUE])*(255-mix)
									+ ((uint16)so->color.blue)*(mix)
									) / 255 );
				
				realDest[L_ALPHA] = (uint8)( ( ((uint16)realDest[L_ALPHA])*(255-so->color.alpha)
									+ ((uint16)255)*(so->color.alpha)
									) / 255 );
			}
			
		}
		
		sourcePos = sourceEOL;
		destPos = destEOL;
	}
	
	return B_OK;
}

#endif
		
BBitmapButton::BBitmapButton(BRect frame, const char* name,
							  const char* label,
							  BMessage* message,
							  const BBitmap* bmNormal,
							  const BBitmap* bmOver,
							  const BBitmap* bmPressed,
							  const BBitmap* bmDisabled,
							  const BBitmap* bmDisabledPressed,
							  uint32 resizeMask)
	: BButton(frame, name, label, message, resizeMask,
			  B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE),
	  fRawBitmap(bmNormal),
	  fNormalBitmap(bmNormal), fOverBitmap(bmOver), fPressedBitmap(bmPressed),
	  fDisabledBitmap(bmDisabled), fDisabledPressedBitmap(bmDisabledPressed),
	  fMadeNormal(false), fMadeOver(false), fMadePressed(false),
	  fMadeDisabled(false), fMadeDisabledPressed(false),
	  fLabelPosition(LABEL_BOTTOM),
	  fMousePressed(false), fMouseOver(false), fSettingValue(false)
{
	if( !fOverBitmap && !fPressedBitmap &&
			!fDisabledBitmap && !fDisabledPressedBitmap ) {
		fNormalBitmap = 0;
	}
}

BBitmapButton::~BBitmapButton()
{
	SetNormalBitmap(0);
	SetOverBitmap(0);
	SetPressedBitmap(0);
	SetDisabledBitmap(0);
	SetDisabledPressedBitmap(0);
}

void BBitmapButton::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( Parent() ) {
		BFont font;
		Parent()->GetFont(&font);
		SetFont(&font);
		SetViewColor(Parent()->ViewColor());
		SetLowColor(Parent()->LowColor());
		SetHighColor(Parent()->HighColor());
	}
}

void BBitmapButton::MouseDown(BPoint where)
{
	fMousePressed = true;
	inherited::MouseDown(where);
}

void BBitmapButton::MouseMoved(BPoint where, uint32 transit, const BMessage *drag)
{
	if( NormalBitmap() ) {
		bool inside = Bounds().Contains(where);
		if( inside != fMouseOver ) {
			fMouseOver = inside;
			if( IsEnabled() ) {
				Invalidate();
			}
		}
	}
	
	inherited::MouseMoved(where, transit, drag);
}

void BBitmapButton::MouseUp(BPoint where)
{
	fMousePressed = false;
	inherited::MouseUp(where);
}

#if AUTO_BITMAP_GENERATION

static const shadow_offset default_shadows[] = {
	{ B_SHADOW_INTERIOR, { 255, 255, 255, 168 }, 0, 1 },
	{ B_SHADOW_INTERIOR, { 255, 255, 255, 168 }, 1, 0 },
	{ B_SHADOW_INTERIOR, { 0, 0, 0, 128 }, -1, 0 },
	{ B_SHADOW_INTERIOR, { 0, 0, 0, 128 }, 0, -1 },
#if 0
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, -1, 0 },
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, 0, -1 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 1, 0 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 0, 1 },
#else
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, -1, -1 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 1, 1 },
#endif
	{ B_SHADOW_END, { 0, 0, 0, 0 }, 0, 0 }
};

#if FANCY_OVER_SHADOWS
static const shadow_offset over_pre_shadows[] = {
	{ B_SHADOW_INTERIOR, { 255, 255, 255, 168 }, 0, 1 },
	{ B_SHADOW_INTERIOR, { 255, 255, 255, 168 }, 1, 0 },
	{ B_SHADOW_INTERIOR, { 0, 0, 0, 128 }, -1, 0 },
	{ B_SHADOW_INTERIOR, { 0, 0, 0, 128 }, 0, -1 },
	{ B_SHADOW_END, { 0, 0, 0, 0 }, 0, 0 }
};
#endif

static const shadow_offset over_shadows[] = {
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, -1, 0 },
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, 0, -1 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 0, 1 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 1, 0 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 2, 2 },
	{ B_SHADOW_END, { 0, 0, 0, 0 }, 0, 0 }
};

static const shadow_offset pressed_pre_shadows[] = {
	{ B_SHADOW_INTERIOR, { 0, 0, 0, 64 }, 3, 3 },
	{ B_SHADOW_INTERIOR, { 255, 255, 255, 128 }, 0, 1 },
	{ B_SHADOW_INTERIOR, { 255, 255, 255, 128 }, 1, 0 },
	{ B_SHADOW_END, { 0, 0, 0, 0 }, 1, 1 }
};

static const shadow_offset pressed_shadows[] = {
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 0, 1 },
	{ B_SHADOW_EXTERIOR, { 0, 0, 0, 64 }, 1, 0 },
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, 1, 2 },
	{ B_SHADOW_EXTERIOR, { 255, 255, 255, 128 }, 2, 1 },
	{ B_SHADOW_END, { 0, 0, 0, 0 }, 1, 1 }
};

static const shadow_offset disabled_shadows[] = {
	{ B_SHADOW_END, { 0, 0, 0, 0 }, 0, 0 }
};

#endif

void BBitmapButton::Draw(BRect updateRect)
{
	if( NormalBitmap() ) {
		if( fSettingValue ) {
			Invalidate(updateRect);
			return;
		}
		
		PushState();
		
		font_height fh;
		float lw;
		const char* l = Label();
		if( l && *l ) {
			lw = StringWidth(l);
			GetFontHeight(&fh);
			fh.descent += 2;
			fh.ascent += 1;
		} else {
			fh.ascent = fh.descent = fh.leading = 0;
			lw = 0;
		}
		
		const BBitmap* bm = NormalBitmap();
#if AUTO_BITMAP_GENERATION
		const shadow_offset* s = default_shadows;
#endif
		if( IsEnabled() ) {
			if( fMouseOver ) {
				if( fMousePressed ) {
					bm = PressedBitmap();
#if AUTO_BITMAP_GENERATION
					s = pressed_shadows;
#endif
				} else {
					bm = OverBitmap();
#if AUTO_BITMAP_GENERATION
					s = over_shadows;
#endif
				}
			} else {
				if( Value() ) {
					bm = PressedBitmap();
#if AUTO_BITMAP_GENERATION
					s = pressed_shadows;
#endif
				}
			}
		} else {
			if( Value() ) bm = DisabledPressedBitmap();
			else bm = DisabledBitmap();
#if AUTO_BITMAP_GENERATION
			s = disabled_shadows;
#endif
		}
		
		BRect bounds(Bounds());
		
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		float bmX = floor((bounds.Width()-bm->Bounds().Width())/2);
		DrawBitmap(bm, BPoint(bmX, 0));
		
		bool isFocus = IsFocus() && Window()->IsActive();
		
		if( l && *l ) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			SetLowColor(ViewColor());
			rgb_color textColor;
			if( IsEnabled() ) {
				textColor = HighColor();
			} else {
				textColor = disable_color(HighColor(), ViewColor());
			}
			
			BPoint where(floor((bounds.Width()-lw)/2), bounds.bottom-fh.descent);
#if AUTO_BITMAP_GENERATION
			while( s ) {
				if( s->op == B_SHADOW_EXTERIOR ) {
					SetHighColor(s->color);
				} else if( s->op == B_SHADOW_END ) {
					SetHighColor(textColor);
				} else {
					s++;
					continue;
				}
				DrawString(l, BPoint(where.x + s->x_offset, where.y + s->y_offset));
				if( s->op == B_SHADOW_END ) break;
				s++;
			}
#else
			SetHighColor(textColor);
			DrawString(l, where);
#endif

			if( isFocus ) {
				SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
				StrokeLine(where, BPoint(where.x+lw, where.y));
				SetHighColor(255,255,255);
				StrokeLine(BPoint(where.x+1, where.y+1), BPoint(where.x+lw+1, where.y+1));
			}
		} else if( isFocus ) {
			BPoint where(bmX, bm->Bounds().bottom);
			if( where.y >= bounds.bottom ) where.y = bounds.bottom-1;
			float w = bm->Bounds().Width();
			SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
			StrokeLine(where, BPoint(where.x+w, where.y));
			SetHighColor(255,255,255);
			StrokeLine(BPoint(where.x+1, where.y+1), BPoint(where.x+w+1, where.y+1));
		}
		
		PopState();
		
	} else {
		inherited::Draw(updateRect);
	}
}

void BBitmapButton::GetPreferredSize(float* width, float* height)
{
	if( NormalBitmap() ) {
		BRect b = NormalBitmap()->Bounds();
		*width = b.Width();
		*height = b.Height();
		const char* l = Label();
		if( l && *l ) {
			font_height fh;
			GetFontHeight(&fh);
			float w = StringWidth(l)+4;
			float h = fh.leading+fh.ascent+fh.descent+4;
			if( LabelPosition() == LABEL_BOTTOM || LabelPosition() == LABEL_TOP ) {
				if( w > *width ) *width = w;
				*height += h;
			} else {
				*width += w;
				if( h > *height ) *height = h;
			}
		}
	} else {
		inherited::GetPreferredSize(width, height);
	}
}

void BBitmapButton::MakeFocus(bool focusState)
{
	if( NormalBitmap() ) {
		fSettingValue = true;
	}
	
	inherited::MakeFocus(focusState);
	
	fSettingValue = false;
}

void BBitmapButton::SetValue(int32 value)
{
	if( NormalBitmap() ) {
		fSettingValue = true;
	}
	
	inherited::SetValue(value);
	
	fSettingValue = false;
}

void BBitmapButton::SetEnabled(bool on)
{
	if( NormalBitmap() ) {
		fSettingValue = true;
	}
	
	inherited::SetEnabled(on);
	
	fSettingValue = false;
}

const BBitmap* BBitmapButton::NormalBitmap(bool create) const
{
	(void)create;
	
	if( fNormalBitmap ) return fNormalBitmap;
	if( create && RawBitmap() ) {
		BBitmapButton* This = const_cast<BBitmapButton*>(this);
		
#if AUTO_BITMAP_GENERATION
		BBitmap* bm = new BBitmap(RawBitmap());
		if( shadow_bitmap(bm, default_shadows, RawBitmap()) == B_OK ) {
			This->fNormalBitmap = bm;
			This->fMadeNormal = true;
		} else {
			delete bm;
			This->fNormalBitmap = fRawBitmap;
			This->fMadeNormal = false;
		}
#else
		This->fNormalBitmap = fRawBitmap;
		This->fMadeNormal = false;
#endif
		
		return This->fNormalBitmap;
		
	}
	return 0;
}

const BBitmap* BBitmapButton::OverBitmap(bool create) const
{
	if( fOverBitmap ) return fOverBitmap;
	if( create && RawBitmap() ) {
		BBitmapButton* This = const_cast<BBitmapButton*>(this);
		
#if AUTO_BITMAP_GENERATION
		rgb_color shine = { 255, 255, 255, 64 };
		
		BBitmap* bm = new BBitmap(RawBitmap());
		if( tint_bitmap(bm, shine) == B_OK &&
#if FANCY_OVER_SHADOWS
				shadow_bitmap(bm, over_pre_shadows, RawBitmap()) == B_OK &&
#endif
				shadow_bitmap(bm, over_shadows, RawBitmap()) == B_OK ) {
			This->fOverBitmap = bm;
			This->fMadeOver = true;
		} else {
			delete bm;
			This->fOverBitmap = fRawBitmap;
			This->fMadeOver = false;
		}
#else
		This->fOverBitmap = fRawBitmap;
		This->fMadeOver = false;
#endif
		
		return This->fOverBitmap;
	}
	return 0;
}

const BBitmap* BBitmapButton::PressedBitmap(bool create) const
{
	if( fPressedBitmap ) return fPressedBitmap;
	if( create && RawBitmap() ) {
		BBitmapButton* This = const_cast<BBitmapButton*>(this);
		
#if AUTO_BITMAP_GENERATION
		BBitmap* bm = new BBitmap(RawBitmap()->Bounds(), 0,
								  RawBitmap()->ColorSpace(),
								  RawBitmap()->BytesPerRow());
		uint32* bits = (uint32*)bm->Bits();
		uint32* bits_end = bits + RawBitmap()->BitsLength()/4;
		while( bits && bits < bits_end ) {
			*bits++ = B_TRANSPARENT_MAGIC_RGBA32;
		}
		
		BRect src = RawBitmap()->Bounds();
		src.right -= 1;
		src.bottom -= 1;
		rgb_color shadow = { 0, 0, 0, 64 };
		
		if( copy_bitmap(bm, RawBitmap(), src, BPoint(1, 1)) == B_OK &&
				tint_bitmap(bm, shadow) == B_OK &&
				shadow_bitmap(bm, pressed_pre_shadows, RawBitmap()) == B_OK &&
				shadow_bitmap(bm, pressed_shadows, RawBitmap()) == B_OK ) {
			This->fPressedBitmap = bm;
			This->fMadePressed = true;
		} else {
			delete bm;
			This->fPressedBitmap = fRawBitmap;
			This->fMadePressed = false;
		}
#else
		This->fPressedBitmap = fRawBitmap;
		This->fMadePressed = false;
#endif
		
		return This->fPressedBitmap;
	}
	return 0;
}

const BBitmap* BBitmapButton::DisabledBitmap(bool create) const
{
	if( fDisabledBitmap ) return fDisabledBitmap;
	if( create && RawBitmap() ) {
		BBitmapButton* This = const_cast<BBitmapButton*>(this);
		
#if AUTO_BITMAP_GENERATION
		BBitmap* bm = new BBitmap(RawBitmap());
		rgb_color background = { 0, 0, 0, 0 };
		if( disable_bitmap(bm, background) == B_OK ) {
			This->fDisabledBitmap = bm;
			This->fMadeDisabled = true;
		} else {
			delete bm;
			This->fDisabledBitmap = fRawBitmap;
			This->fMadeDisabled = false;
		}
#else
		This->fDisabledBitmap = fRawBitmap;
		This->fMadeDisabled = false;
#endif
		
		return This->fDisabledBitmap;
	}
	return 0;
}

const BBitmap* BBitmapButton::DisabledPressedBitmap(bool create) const
{
	if( fDisabledPressedBitmap ) return fDisabledPressedBitmap;
	const BBitmap* pressed;
	if( create && (pressed=PressedBitmap(create)) ) {
		BBitmapButton* This = const_cast<BBitmapButton*>(this);
		
#if AUTO_BITMAP_GENERATION
		BBitmap* bm = new BBitmap(pressed);
		rgb_color background = { 0, 0, 0, 0 };
		if( disable_bitmap(bm, background) == B_OK ) {
			This->fDisabledPressedBitmap = bm;
			This->fMadeDisabled = true;
		} else {
			delete bm;
			This->fDisabledPressedBitmap = fRawBitmap;
			This->fMadeDisabled = false;
		}
#else
		This->fDisabledPressedBitmap = DisabledBitmap(create);
		This->fMadeDisabledPressed = false;
#endif
		
		return This->fDisabledPressedBitmap;
	}
	if( create ) return fNormalBitmap;
	return 0;
}

void BBitmapButton::SetNormalBitmap(const BBitmap* bmap)
{
	if( fOverBitmap == fNormalBitmap ) fOverBitmap = 0;
	if( fPressedBitmap == fNormalBitmap ) fPressedBitmap = 0;
	if( fDisabledBitmap == fNormalBitmap ) fDisabledBitmap = 0;
	if( fDisabledPressedBitmap == fNormalBitmap ) fDisabledPressedBitmap = 0;
	
	if( fMadeNormal ) delete const_cast<BBitmap*>(fNormalBitmap);
	fNormalBitmap = 0;
	
	fRawBitmap = bmap;
	fMadeNormal = false;
}

void BBitmapButton::SetOverBitmap(const BBitmap* bmap)
{
	if( fMadeOver ) delete const_cast<BBitmap*>(fOverBitmap);
	fOverBitmap = bmap;
	fMadeOver = false;
}

void BBitmapButton::SetPressedBitmap(const BBitmap* bmap)
{
	if( fMadePressed ) delete const_cast<BBitmap*>(fPressedBitmap);
	fPressedBitmap = bmap;
	fMadePressed = false;
}

void BBitmapButton::SetDisabledBitmap(const BBitmap* bmap)
{
	if( fMadeDisabled ) delete const_cast<BBitmap*>(fDisabledBitmap);
	fDisabledBitmap = bmap;
	fMadeDisabled = false;
}

void BBitmapButton::SetDisabledPressedBitmap(const BBitmap* bmap)
{
	if( fMadeDisabledPressed ) delete const_cast<BBitmap*>(fDisabledPressedBitmap);
	fDisabledPressedBitmap = bmap;
	fMadeDisabledPressed = false;
}

void BBitmapButton::SetLabelPosition(label_position pos)
{
	if( fLabelPosition != pos ) {
		fLabelPosition = pos;
		Invalidate();
	}
}

BBitmapButton::label_position BBitmapButton::LabelPosition() const
{
	return fLabelPosition;
}

const BBitmap* BBitmapButton::RawBitmap() const
{
	return fRawBitmap;
}
