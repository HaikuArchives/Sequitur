/* ArpPrefsI.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This file defines classes that provide global, application-wide
 * behaviour, along with functions for accessing instances of each
 * of these classes.  The application implementing this library needs
 * to implement the accessing functions.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.08.00		hackborn
 * Created this file.
 */

#ifndef AMPUBLIC_AMPREFSI_H
#define AMPUBLIC_AMPREFSI_H

#include "ArpViewsPublic/ArpPrefsI.h"

	/*---------------------------------------------------------
	 * INT32 CONSTANTS.  Access with Prefs().Int32().
	 *---------------------------------------------------------*/

enum {
	AM_FILTER_BG_COUNT_I32		= ARP_USER_INT32_BASE + 1,
	
	AM_INT32_PREF_END			= ARP_USER_INT32_BASE + 2
};

	/*---------------------------------------------------------
	 * SIZE CONSTANTS.  Access with Prefs().Size().
	 *---------------------------------------------------------*/

enum {
	AM_PHRASE_LABEL_Y			= ARP_USER_SIZE_BASE,
	
	AM_SIZE_PREF_END			= ARP_USER_SIZE_BASE + 1
};

	/*---------------------------------------------------------
	 * COLOUR CONSTANTS.  Access with Prefs().Color().
	 *---------------------------------------------------------*/

enum {
	AM_CONTROL_BG_C				= ARP_USER_COLOR_BASE,
	AM_CONTROL_FG_C				= ARP_USER_COLOR_BASE + 1,
	AM_LCD_C					= ARP_USER_COLOR_BASE + 2,

	AM_MEASURE_TOP_BG_C			= ARP_USER_COLOR_BASE + 3,
	AM_MEASURE_BOTTOM_BG_C		= ARP_USER_COLOR_BASE + 4,
	AM_MEASURE_LEFT_BG_C		= ARP_USER_COLOR_BASE + 5,
	AM_MEASURE_RIGHT_BG_C		= ARP_USER_COLOR_BASE + 6,
	AM_MEASURE_FG_C				= ARP_USER_COLOR_BASE + 7,
	AM_MEASURE_BEAT_C			= ARP_USER_COLOR_BASE + 8,
	AM_MEASURE_HIGHLIGHT_C		= ARP_USER_COLOR_BASE + 9,
	AM_GRID_C					= ARP_USER_COLOR_BASE + 10,

	AM_DATA_BACKDROP_C			= ARP_USER_COLOR_BASE + 11,
	AM_DATA_BG_C				= ARP_USER_COLOR_BASE + 12,
	AM_DATA_FG_C				= ARP_USER_COLOR_BASE + 13,
	AM_INFO_BG_C				= ARP_USER_COLOR_BASE + 14,
	AM_HIGH_PRIMARY_EVENT_C		= ARP_USER_COLOR_BASE + 15,
	AM_LOW_PRIMARY_EVENT_C		= ARP_USER_COLOR_BASE + 16,
	AM_HIGH_SECONDARY_EVENT_C	= ARP_USER_COLOR_BASE + 17,
	AM_LOW_SECONDARY_EVENT_C	= ARP_USER_COLOR_BASE + 18,
	AM_HIGH_SELECTED_EVENT_C	= ARP_USER_COLOR_BASE + 19,
	AM_LOW_SELECTED_EVENT_C		= ARP_USER_COLOR_BASE + 20,
	
	AM_ARRANGE_BG_C				= ARP_USER_COLOR_BASE + 21,
	AM_ARRANGE_FG_C				= ARP_USER_COLOR_BASE + 22,
	AM_SONG_SELECTION_C			= ARP_USER_COLOR_BASE + 23,
	AM_SONG_RECORD_SELECTION_C	= ARP_USER_COLOR_BASE + 24,
	
	AM_PIPELINE_C				= ARP_USER_COLOR_BASE + 25,
	
	AM_AUX_WINDOW_BG_C			= ARP_USER_COLOR_BASE + 26,

	DUPLICATE_FILTER_1_C		= ARP_USER_COLOR_BASE + 27,
	DUPLICATE_FILTER_2_C		= ARP_USER_COLOR_BASE + 28,
	DUPLICATE_FILTER_3_C		= ARP_USER_COLOR_BASE + 29,
	DUPLICATE_FILTER_4_C		= ARP_USER_COLOR_BASE + 30,
	DUPLICATE_FILTER_5_C		= ARP_USER_COLOR_BASE + 31,
	DUPLICATE_FILTER_6_C		= ARP_USER_COLOR_BASE + 32,
	DUPLICATE_FILTER_7_C		= ARP_USER_COLOR_BASE + 33,
	DUPLICATE_FILTER_8_C		= ARP_USER_COLOR_BASE + 34,
	DUPLICATE_FILTER_9_C		= ARP_USER_COLOR_BASE + 35,
	
	AM_COLOR_PREF_END			= ARP_USER_COLOR_BASE + 36
};

/***************************************************************************
 * AM-PREFERENCES-I
 * An interface required by some of the AmKernel view classes.
 ***************************************************************************/
// Forward reference
class AmPreferencesI;
// Accessing function
AmPreferencesI& AmPrefs();
// Let application attach preferences object
void SetAmPrefs(AmPreferencesI& prefs);

class AmPreferencesI : public ArpPreferencesI
{
public:
	virtual ~AmPreferencesI()	{ }
	
	/* Answer a color somewhere between AM_LOW_PRIMARY_EVENT_C and
	 * AM_HIGH_PRIMARY_EVENT_C, depending on the value of velocity.
	 * A velocity of 0 is the low colour, 127 is the high colour.
	 */
	virtual rgb_color	PrimaryColor(uint8 velocity = 127) const = 0;
	/* Answer a color somewhere between AM_LOW_SECONDARY_EVENT_C and
	 * AM_HIGH_SECONDARY_EVENT_C, depending on the value of velocity.
	 * A velocity of 0 is the low colour, 127 is the high colour.
	 */
	virtual rgb_color	ShadowColor(uint8 velocity = 127) const = 0;
	/* Answer a color somewhere between AM_LOW_SELECTED_EVENT_C and
	 * AM_HIGH_SELECTED_EVENT_C, depending on the value of velocity.
	 * A velocity of 0 is the low colour, 127 is the high colour.
	 */
	virtual rgb_color	SelectedColor(uint8 velocity = 127) const = 0;

	virtual status_t	GetFactoryInt32(const char* fac, const char* view,
										const char* name, int32* outI32, int32 n = 0) const = 0;
};

#endif
