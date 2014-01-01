#ifndef ARPINTERFACESUPPORT_ARPPREFS_H
#define ARPINTERFACESUPPORT_ARPPREFS_H

#include <be/interface/GraphicsDefs.h>
#include <be/interface/Point.h>
#include <ArpInterface/ArpFont.h>

class ArpPrefs;

const ArpPrefs&		Prefs();
// The application sets the prefs.
void				SetPrefs(ArpPrefs& prefs);

/*******************************************************
 * ARP-PREFS
 * A repository for application-defined preferences.
 *******************************************************/
class ArpPrefs
{
public:
	virtual ~ArpPrefs()		{ }

	virtual int32			GetInt32(uint32 constant) const = 0;
	virtual float			GetFloat(uint32 constant) const = 0;
	virtual BPoint			GetPoint(uint32 constant) const = 0;
	virtual rgb_color		GetColor(uint32 constant) const = 0;
	virtual const ArpFont*	GetFont(uint32 constant) const = 0;
};


/* Int32 constants
 */
enum {
	ARP_FULLFONT_Y	= 0,
	ARP_INTCTRL_Y	= 1,
	ARP_MENUCTRL_Y	= 2,
	ARP_TEXTCTRL_Y	= 3,
	ARP_CHECKBOX_Y	= 4,
	ARP_BUTTON_Y	= 5,

	ARP_PADX		= 10,	// X-space padding
	ARP_PADY		= 11,	// Y-space padding

	ARP_INT32_PREF_SIZE
};


/* Float constants
 */
enum {
	ARP_PADX_F		= 0,
	ARP_PADY_F		= 1,

	ARP_FLOAT_PREF_SIZE
};

/* Color constants
 */
enum {
	ARP_BG_C		= 0,
	ARP_FG_C		= 1,
	ARP_INT_BG_C	= 2,		// Background for int controls
	ARP_INT_BGF_C	= 3,		// Background for int controls with focus
	ARP_INT_FG_C	= 4,		// Foreground for int controls
	ARP_INT_FGF_C	= 5,		// Foreground for int controls with focus
	ARP_WIN_BG_C	= 6,		// Background for windows

	ARP_COLOR_PREF_SIZE
};

/* FONT constants
 */
enum {
	ARP_CONTROL_FONT	= 0,

	ARP_FONT_PREF_SIZE
};


#endif
