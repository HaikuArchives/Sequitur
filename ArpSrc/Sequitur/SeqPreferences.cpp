/* SeqPreferences.cpp
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <interface/Font.h>
#include <interface/InterfaceDefs.h>
#include <interface/StatusBar.h>
#include <support/TypeConstants.h>
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmDefs.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqPreferences.h"

/*************************************************************************
 * SEQ-PREFERENCES
 *************************************************************************/
SeqPreferences::SeqPreferences()
{
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

	mPrefSizes[AM_PHRASE_LABEL_Y] = 8;

	SetPrefs(*this);
	SetAmPrefs(*this);
}

void SeqPreferences::Initialize()
{
	// --------- INT32s ---------
	mPrefInt32[AM_FILTER_BG_COUNT_I32] = 0;
	BString			str(FILTER_BG_PREFIX);
	str << mPrefInt32[AM_FILTER_BG_COUNT_I32];
	while (Resources().FindBitmap(str.String()) != NULL) {
		mPrefInt32[AM_FILTER_BG_COUNT_I32]++;
		str = FILTER_BG_PREFIX;
		str << mPrefInt32[AM_FILTER_BG_COUNT_I32];
	}

	// --------- COLORS ---------

	SetColor(INT_CTRL_BG_C, tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	SetColor(INT_CTRL_BGF_C, 255, 255, 255);
	SetColor(INT_CTRL_FG_C, 0, 0, 0);
	SetColor(INT_CTRL_FGF_C, 0, 0, 0);

	/* ------- Default colors should be overwritten from my resource file ------ */
	SetColor(AM_CONTROL_BG_C,			180, 180, 190);
	SetColor(AM_CONTROL_FG_C,			0, 0, 0);
	SetColor(AM_LCD_C,					120, 120, 138);

	SetColor(AM_MEASURE_TOP_BG_C,		171, 171, 189);
	SetColor(AM_MEASURE_BOTTOM_BG_C,	147, 147, 165);
	SetColor(AM_MEASURE_LEFT_BG_C,		90,  90,  90);
	SetColor(AM_MEASURE_RIGHT_BG_C,		90,  90,  90);
	SetColor(AM_MEASURE_FG_C,			0,   0,   0);
	SetColor(AM_MEASURE_BEAT_C,			200, 200, 200);
	SetColor(AM_MEASURE_HIGHLIGHT_C,	200, 200, 200);
	SetColor(AM_GRID_C,					220, 220, 220);

	SetColor(AM_DATA_BACKDROP_C, 		140, 140, 150);
	SetColor(AM_DATA_BG_C,				225, 225, 225);
	SetColor(AM_DATA_FG_C,				0,   0,   0);
	SetColor(AM_INFO_BG_C,				170, 170, 170);

	SetColor(AM_ARRANGE_BG_C,			170, 170, 170);
	SetColor(AM_ARRANGE_FG_C,			0,   0,   0);
	SetColor(AM_SONG_SELECTION_C,		255, 255, 0);
	SetColor(AM_SONG_RECORD_SELECTION_C,	255,   0,   0);

	SetColor(AM_PIPELINE_C,				200, 200, 200);

	SetColor(AM_AUX_WINDOW_BG_C,		ui_color(B_PANEL_BACKGROUND_COLOR) );

	/* The event colors
	 */
	mColors[AM_HIGH_PRIMARY_EVENT_C].red = 70;
	mColors[AM_HIGH_PRIMARY_EVENT_C].green = 70;
	mColors[AM_HIGH_PRIMARY_EVENT_C].blue = 200;
	mColors[AM_HIGH_PRIMARY_EVENT_C].alpha = 255;
	SetColor(AM_LOW_PRIMARY_EVENT_C,	200, 200, 255);

	mColors[AM_HIGH_SECONDARY_EVENT_C].red = 160;
	mColors[AM_HIGH_SECONDARY_EVENT_C].green = 160;
	mColors[AM_HIGH_SECONDARY_EVENT_C].blue = 160;
	mColors[AM_HIGH_SECONDARY_EVENT_C].alpha = 255;
	SetColor(AM_LOW_SECONDARY_EVENT_C,	208, 208, 208);

	mColors[AM_HIGH_SELECTED_EVENT_C].red = 200;
	mColors[AM_HIGH_SELECTED_EVENT_C].green = 20;
	mColors[AM_HIGH_SELECTED_EVENT_C].blue = 20;
	mColors[AM_HIGH_SELECTED_EVENT_C].alpha = 255;
	SetColor(AM_LOW_SELECTED_EVENT_C,	255, 200, 200);

	SetColor(DUPLICATE_FILTER_1_C,		100, 100, 255, 64);
	SetColor(DUPLICATE_FILTER_2_C,		0, 0, 255, 64);
	SetColor(DUPLICATE_FILTER_3_C,		100, 0, 255, 64);
	SetColor(DUPLICATE_FILTER_4_C,		255, 0, 255, 64);
	SetColor(DUPLICATE_FILTER_5_C,		255, 0, 100, 64);
	SetColor(DUPLICATE_FILTER_6_C,		255, 0, 0, 64);
	SetColor(DUPLICATE_FILTER_7_C,		255, 100, 0, 64);
	SetColor(DUPLICATE_FILTER_8_C,		255, 255, 0, 64);
	SetColor(DUPLICATE_FILTER_9_C,		100, 255, 0, 64);

	// --------- IMAGES ---------

	const BBitmap *bm = ImageManager().FindBitmap( KNOB_000_IMAGE_STR );
	if( bm ) {
		mPrefSizes[KNOB_X] = bm->Bounds().Width() + 1;
		mPrefSizes[KNOB_Y] = bm->Bounds().Height() + 1;
	}
	bm = ImageManager().FindBitmap( KNOB_RING_IMAGE_STR );
	if( bm ) {
		mPrefSizes[KNOB_RING_X] = bm->Bounds().Width() + 1;
		mPrefSizes[KNOB_RING_Y] = bm->Bounds().Height() + 1;
	}
	bm = ImageManager().FindBitmap( KNOB_RING_TIGHT_IMAGE_STR );
	if( bm ) {
		mPrefSizes[KNOB_RING_TIGHT_X] = bm->Bounds().Width() + 1;
		mPrefSizes[KNOB_RING_TIGHT_Y] = bm->Bounds().Height() + 1;
	}
	bm = ImageManager().FindBitmap( SLICE_PROPERTY_MENU_NORMAL_IMAGE_STR );
	if( bm ) {
		mPrefSizes[PROP_FIELD_X] = bm->Bounds().Width();
		mPrefSizes[PROP_FIELD_Y] = bm->Bounds().Height();
	}

	size_t			size;
	const void*		res = Resources().FindResource(B_RGB_COLOR_TYPE, "Control BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_CONTROL_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Control FG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_CONTROL_FG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Measure Top BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_MEASURE_TOP_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Measure Bottom BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_MEASURE_BOTTOM_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	/* This is interpolated from the above two values.
	 */
	rgb_color		c = mix_color(mColors[AM_MEASURE_TOP_BG_C], mColors[AM_MEASURE_BOTTOM_BG_C], 127);
	SetColor( AM_MEASURE_HIGHLIGHT_C, tint_color(c, B_LIGHTEN_1_TINT) );

	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Measure Left BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_MEASURE_LEFT_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Measure Right BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_MEASURE_RIGHT_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Measure FG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_MEASURE_FG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Measure Beat", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_MEASURE_BEAT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Grid", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_GRID_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Data Backdrop", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_DATA_BACKDROP_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Data BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_DATA_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Info BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_INFO_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Data FG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_DATA_FG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Arrange BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_ARRANGE_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Arrange FG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_ARRANGE_FG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Song Selection", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_SONG_SELECTION_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, ((rgb_color*)res)->alpha);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Song Record Selection", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_SONG_RECORD_SELECTION_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, ((rgb_color*)res)->alpha);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Pipeline", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_PIPELINE_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, ((rgb_color*)res)->alpha);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Auxiliary Window BG", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_AUX_WINDOW_BG_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Primary Event High", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_HIGH_PRIMARY_EVENT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Primary Event Low", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_LOW_PRIMARY_EVENT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Secondary Event High", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_HIGH_SECONDARY_EVENT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Secondary Event Low", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_LOW_SECONDARY_EVENT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Selected Event High", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_HIGH_SELECTED_EVENT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Selected Event Low", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(AM_LOW_SELECTED_EVENT_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue);

	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 1", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_1_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 2", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_2_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 3", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_3_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 4", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_4_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 5", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_5_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 6", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_6_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 7", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_7_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 8", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_8_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
	res = Resources().FindResource(B_RGB_COLOR_TYPE, "Duplicate Filter 9", &size);
	if( res && size == sizeof(rgb_color) ) SetColor(DUPLICATE_FILTER_9_C, ((rgb_color*)res)->red, ((rgb_color*)res)->green, ((rgb_color*)res)->blue, 64);
}

int32 SeqPreferences::Int32(uint32 constant) const
{
	if (constant >= INT32_LIMIT) return 0;
	return mPrefInt32[constant];
}

float SeqPreferences::Size(uint32 constant) const
{
	return mPrefSizes[constant];
}

rgb_color SeqPreferences::Color(uint32 constant) const
{
	return mColors[constant];
}

rgb_color SeqPreferences::PrimaryColor(uint8 velocity) const
{
	if( velocity > 127 ) return mPrimary[127];
	return mPrimary[velocity];
}

rgb_color SeqPreferences::ShadowColor(uint8 velocity) const
{
	if( velocity > 127 ) return mSecondary[127];
	return mSecondary[velocity];
}

rgb_color SeqPreferences::SelectedColor(uint8 velocity) const
{
	if( velocity > 127 ) return mSelected[127];
	return mSelected[velocity];
}

status_t SeqPreferences::GetFactoryInt32(	const char* fac, const char* view,
											const char* name, int32* outI32, int32 n) const
{
	return seq_get_factory_int32_preference(fac, view, name, outI32, n);
}

void SeqPreferences::SetColor(	uint32 constant,
								uint8 red,
								uint8 green,
								uint8 blue,
								uint8 alpha)
{
	mColors[constant].red = red;
	mColors[constant].green = green;
	mColors[constant].blue = blue;
	mColors[constant].alpha = alpha;

	if( constant == AM_HIGH_PRIMARY_EVENT_C || constant == AM_LOW_PRIMARY_EVENT_C )
		SetColorArray(	mColors[AM_HIGH_PRIMARY_EVENT_C],
						mColors[AM_LOW_PRIMARY_EVENT_C],
						mPrimary );
	else if( constant == AM_HIGH_SECONDARY_EVENT_C || constant == AM_LOW_SECONDARY_EVENT_C )
		SetColorArray(	mColors[AM_HIGH_SECONDARY_EVENT_C],
						mColors[AM_LOW_SECONDARY_EVENT_C],
						mSecondary );
	else if( constant == AM_HIGH_SELECTED_EVENT_C || constant == AM_LOW_SELECTED_EVENT_C )
		SetColorArray(	mColors[AM_HIGH_SELECTED_EVENT_C],
						mColors[AM_LOW_SELECTED_EVENT_C],
						mSelected );
}

void SeqPreferences::SetColor(	uint32 constant,
								rgb_color color)
{
	SetColor( constant, color.red, color.green, color.blue, color.alpha );
}

void SeqPreferences::SetColorArray(rgb_color highC, rgb_color lowC, rgb_color* colorArray)
{
	float	redInc = (float)(highC.red - lowC.red) / (COLOR_ARRAY_SIZE - 2),
			greenInc = (float)(highC.green - lowC.green) / (COLOR_ARRAY_SIZE - 2),
			blueInc = (float)(highC.blue - lowC.blue) / (COLOR_ARRAY_SIZE - 2);

	colorArray[0] = lowC;
	colorArray[0].alpha = 255;
	colorArray[COLOR_ARRAY_SIZE-1] = highC;
	colorArray[COLOR_ARRAY_SIZE-1].alpha = 255;
	for( int32 k = 1; k < (COLOR_ARRAY_SIZE-1); k++ ) {
		colorArray[k].red = (uint8)(lowC.red + (redInc * k));
		colorArray[k].green = (uint8)(lowC.green + (greenInc * k));
		colorArray[k].blue = (uint8)(lowC.blue + (blueInc * k));
		colorArray[k].alpha = 255;
	}
}
