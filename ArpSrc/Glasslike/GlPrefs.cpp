#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <interface/Font.h>
#include <interface/InterfaceDefs.h>
#include <interface/StatusBar.h>
#include <support/TypeConstants.h>
//#include "ArpViewsPublic/ArpViewDefs.h"
#include <GlPublic/GlDefs.h>
#include "Glasslike/GlPrefs.h"

static rgb_color		gWhite;

/*************************************************************************
 * GL-PREFS
 *************************************************************************/
GlPrefs::GlPrefs()
{
	SetPrefs(*this);
}

void GlPrefs::Initialize()
{
	gWhite.red = gWhite.green = gWhite.blue = gWhite.alpha = 255;

	// --------- INT32s ---------
	const BFont*	font = be_plain_font;
	if (font) mInt32s[ARP_FULLFONT_Y] = int32(font->Size());
	mInt32s[ARP_CHECKBOX_Y] = GetInt32(ARP_FULLFONT_Y) + 5;
	mInt32s[ARP_INTCTRL_Y] = GetInt32(ARP_FULLFONT_Y) + 3;

	mInt32s[ARP_MENUCTRL_Y] = GetInt32(ARP_FULLFONT_Y) + 10;
	mInt32s[ARP_TEXTCTRL_Y] = GetInt32(ARP_FULLFONT_Y) + 10;
	mInt32s[ARP_BUTTON_Y] = GetInt32(ARP_FULLFONT_Y) + 14;

	mInt32s[ARP_PADX] = 2;
	mInt32s[ARP_PADY] = 2;

	// --------- Floats ---------
	mFloats[ARP_PADX_F] = 2.0f;
	mFloats[ARP_PADY_F] = 2.0f;

#if 0
	// --------- SIZES ---------
	mPrefSizes[EG_HANDLE_XY] = 3;
	mPrefSizes[FONT_Y] = 10;
	const BFont*	font = be_plain_font;
	if (font) mPrefSizes[FONT_Y] = font->Size();

	mPrefSizes[FULL_FONT_Y] = Size(FONT_Y);
	mPrefSizes[MAINMENU_Y] = Size(FULL_FONT_Y) + 7;
	mPrefSizes[V_SCROLLBAR_X] = 12;
	mPrefSizes[H_SCROLLBAR_Y] = 12;
	mPrefSizes[MENUFIELD_Y] = Size(FONT_Y) + 8;
	mPrefSizes[MENUBAR_Y] = Size(FONT_Y) + 5;
	mPrefSizes[TEXTCTRL_Y] = Size(FONT_Y) + 8;
	mPrefSizes[INT_CTRL_Y] = Size(FONT_Y) + 3;
	mPrefSizes[BOX_CTRL_Y] = Size(FONT_Y) + 5;
	/* FIX:  What's the way to determine the button
	 * height?
	 */
	mPrefSizes[BUTTON_Y] = 24;

	mPrefSizes[BORDER_X] = 5;
	mPrefSizes[BORDER_Y] = 5;
	mPrefSizes[BUTTON_BORDER_X] = 8;
	mPrefSizes[BUTTON_BORDER_Y] = 8;
	mPrefSizes[SPACE_X] = 5;
	mPrefSizes[SPACE_Y] = 5;
	mPrefSizes[BLANK_Y] = 10;
	mPrefSizes[TAB_X] = 15;

	mPrefSizes[KNOB_X] = 0;
	mPrefSizes[KNOB_Y] = 0;
	mPrefSizes[KNOB_RING_X] = 0;
	mPrefSizes[KNOB_RING_Y] = 0;
	mPrefSizes[KNOB_RING_TIGHT_X] = 0;
	mPrefSizes[KNOB_RING_TIGHT_Y] = 0;

	mPrefSizes[CHECK_BOX_X] = 12;
	mPrefSizes[CHECK_BOX_Y] = 12;
	mPrefSizes[PROP_FIELD_X] = 0;
	mPrefSizes[PROP_FIELD_Y] = 0;
#endif

	// --------- COLORS ---------
	SetColor(ARP_BG_C, 180, 180, 180);
	SetColor(ARP_FG_C, 0, 0, 0);
	SetColor(ARP_INT_BG_C, tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	SetColor(ARP_INT_BGF_C, 255, 255, 255);
	SetColor(ARP_INT_FG_C, 0, 0, 0);
	SetColor(ARP_INT_FGF_C, 0, 0, 0);
	SetColor(ARP_WIN_BG_C, tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT));
}

int32 GlPrefs::GetInt32(uint32 constant) const
{
	if (constant < ARP_INT32_PREF_SIZE) return mInt32s[constant];
	if (constant == GL_NODE_IMAGE_X) return 20;
	if (constant == GL_NODE_IMAGE_Y) return 20;
	if (constant == GL_NODE_CONTROL_X) return 10;
	if (constant == GL_NODE_CONTROL_Y) return 10;
	return 0;
}

float GlPrefs::GetFloat(uint32 constant) const
{
	if (constant >= ARP_FLOAT_PREF_SIZE) return 0;
	return mFloats[constant];
}

BPoint GlPrefs::GetPoint(uint32 constant) const
{
	return BPoint(0, 0);
}

rgb_color GlPrefs::GetColor(uint32 constant) const
{
	if (constant >= ARP_COLOR_PREF_SIZE) return gWhite;
	return mColors[constant];
}

const ArpFont* GlPrefs::GetFont(uint32 constant) const
{
	return 0;
}

void GlPrefs::SetColor(	uint32 constant,
						uint8 red, uint8 green,	uint8 blue,	uint8 alpha)
{
	mColors[constant].red = red;
	mColors[constant].green = green;
	mColors[constant].blue = blue;
	mColors[constant].alpha = alpha;
}

void GlPrefs::SetColor(	uint32 constant, rgb_color color)
{
	SetColor(constant, color.red, color.green, color.blue, color.alpha);
}
