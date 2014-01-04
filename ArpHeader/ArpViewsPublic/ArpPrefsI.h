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

#ifndef ARPVIEWSPUBLIC_ARPPREFSI_H
#define ARPVIEWSPUBLIC_ARPPREFSI_H

#include <interface/Bitmap.h>
#include <interface/GraphicsDefs.h>

/***************************************************************************
 * ARP-PREFERENCES-I
 * A class defining the global preferences for the application.
 ***************************************************************************/
// Forward reference
class ArpPreferencesI;
// Accessing function
ArpPreferencesI& Prefs();

// Let application attach preferences object
void SetPrefs(ArpPreferencesI& prefs);

#define ARP_USER_INT32_BASE		(30) // Additional int32s begin here.

// The Size() preferences.  The convention is to postfix any size define
// by X for a width value, Y for a height, and XY for both.
#define EG_HANDLE_XY			(0)	// The distance from the center to the edge of an EG point handle
#define FONT_Y					(1) // The current font size
#define FULL_FONT_Y				(2) // The pixel height of the current font (including the ascent and descent)
#define MAINMENU_Y				(3) // The height that ExMenuBars should be
#define V_SCROLLBAR_X			(4) // The width of a vertical scrollbar
#define H_SCROLLBAR_Y			(5) // The height of a horizontal scrollbar
#define MENUFIELD_Y				(6) // The height that BMenuFields should be
#define MENUBAR_Y				(7) // The height that ExMenuBars should be
#define TEXTCTRL_Y				(8) // The height of a BTextControl
#define INT_CTRL_Y				(9) // The height of int controls.
#define BOX_CTRL_Y				(10) // The height of check box controls.
#define BUTTON_Y				(11) // The height of buttons.

#define BORDER_X				(12) // The standard amount of horizontal space between a control and the
									 // edge of the window
#define BORDER_Y				(13) // Same as BORDER_X but vertical
#define BUTTON_BORDER_X			(14) // Same as BORDER_X but when the window has a button
									 // that is the default (these take up more space)
#define BUTTON_BORDER_Y			(15) // Same as BUTTON_BORDER_X but vertical
#define SPACE_X					(16) // The standard amount of horizontal space between controls
#define SPACE_Y					(17) // Same as SPACE_X but vertical
#define BLANK_Y					(18) // The amount of space to use for blank lines
#define TAB_X					(19) // The amount of space to use for tabs

#define KNOB_X					(20) // The width of a plain, unadorned knob
#define KNOB_Y					(21) // The height of a plain, unadorned knob
#define KNOB_RING_X				(22) // The width of a knob adorned with the ring graphic
#define KNOB_RING_Y				(23) // The height of a knob adorned with the ring graphic
#define KNOB_RING_TIGHT_X		(24) // The width of a knob adorned with the ring graphic
#define KNOB_RING_TIGHT_Y		(25) // The height of a knob adorned with the ring graphic

#define CHECK_BOX_X				(26) // The width of a standard check box
#define CHECK_BOX_Y				(27) // The height of a standard check box
#define PROP_FIELD_X			(28) // The width of the property field
#define PROP_FIELD_Y			(29) // The height of the property field

#define ARP_USER_SIZE_BASE		(50) // Additional sizes begin here.

// The Color() preferences.  The convention is to postfix any color define
// with C.
#define INT_CTRL_BG_C			(12) // The background color for int controls
#define INT_CTRL_BGF_C			(13) // The background color for int controls with focus
#define INT_CTRL_FG_C			(14) // The foreground color for int controls
#define INT_CTRL_FGF_C			(15) // The foreground color for int controls with focus

#define PERCENT_BAR_C			(16) // The color of the bar in the percent status view

#define ARP_USER_COLOR_BASE		(30) // Additional colours begin here.

class ArpPreferencesI
{
public:
	virtual ~ArpPreferencesI()	{ }
	
	virtual void	Initialize()		{ }
	
	virtual int32	Int32(uint32 constant) const = 0;
	/* Answer a pixel size for the supplied constant value.
	 */
	virtual float	Size(uint32 constant) const = 0;
	/* Answer an rgb_color for the supplied constant value.
	 */
	virtual rgb_color Color(uint32 constant) const = 0;
};

/***************************************************************************
 * ARP-IMAGE-MANAGER-I
 * A class that caches all the images in the application, and is therefore
 * responsible for loading and deleting them.  Client access is via name
 * strings.
 *
 * NOTE NOTE NOTE  This class has become a passthrough for the ResourceSet.
 * It's entirely possible this will go away and be replaced by a call
 * to that at some point.
 ***************************************************************************/
// Forward reference
class ArpImageManagerI;
// Accessing function
ArpImageManagerI&	ImageManager();
// Answer true if the image manager has been set, false otherwise.
bool				HasImageManager();

// Let application attach image object
void SetImageManager(ArpImageManagerI& img);

class ArpImageManagerI
{
public:
	virtual ~ArpImageManagerI()	{ }
	
	// Arguments for the PictureAt() method flags arg.
	enum PictureFlags {
		ARP_USE_ALPHA	= 0x00000001
	};

	/* Answer the bitmap at the supplied name.  Name should be
	 * a resource name bound with the project.  Answer 0 if the
	 * bitmap doesn't exist.
	 */
	virtual const BBitmap* FindBitmap(const char *name) const = 0;
};

#endif
