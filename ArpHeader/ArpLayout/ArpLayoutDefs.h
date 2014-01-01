/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * ----------------------------------------------------------------------
 *
 * ArpLayoutTools.h
 *
 * Miscellaneous things used by the ArpLayout library.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * July 31, 1999:
 *	• Created from ArpViewWrapper.h
 *
 */

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_ARPLAYOUTDEFS_H
#define ARPLAYOUT_ARPLAYOUTDEFS_H

// Forward references.
class BMessage;
class BView;
class ArpDimens;
struct param_value_item;

/**	This is an array of param_value_item entries that describe
	the values an orientation parameter can take.  Include this
	in your parameter_info.values entry.
 **/
extern const param_value_item arp_orientation_values[];

/**	This is an array of param_value_item entries that describe
	the values a border_style parameter can take.  Include this
	in your parameter_info.values entry.
 **/
extern const param_value_item arp_border_style_values[];

/**	This is an array of param_value_item entries that describe
	the values an alignment parameter can take.  Include this
	in your parameter_info.values entry.
 **/
extern const param_value_item arp_alignment_values[];

/**	This is an array of param_value_item entries that describe
	the values a vertical_alignment parameter can take.  Include this
	in your parameter_info.values entry.
 **/
extern const param_value_item arp_vertical_alignment_values[];

/**	This type defines where to place an object within a larger
	frame.  It may be centered, in one of the eight compass
	directions, or stretched to fill the larger frame.
 **/
enum ArpGravity {
	ArpCenter				= 0x0000,
	
	ArpNorth				= 0x0001,
	ArpSouth				= 0x0002,
	ArpWest					= 0x0004,
	ArpEast					= 0x0008,

	ArpNorthEast			= ArpNorth|ArpEast,
	ArpNorthWest			= ArpNorth|ArpWest,
	ArpSouthEast			= ArpSouth|ArpEast,
	ArpSouthWest			= ArpSouth|ArpWest,

	ArpNorthSouth			= ArpNorth|ArpSouth,
	ArpEastWest				= ArpEast|ArpWest,

	ArpFillAll				= ArpNorthSouth|ArpEastWest
};

/**	This is an array of param_value_item entries that describe
	the values an ArpGravity parameter can take.  Include this
	in your parameter_info.values entry.
 **/
extern const param_value_item arp_gravity_values[];

/**	Quick way to pass no (a.k.a. default) paramters to a function.
	@defvar ArpNoParams BMessage ArpNoParams
 **/
extern const BMessage ArpNoParams;

/** Copy all values in message 'from' to message 'to'.  If a
	value appears in 'to' but not in 'from', it is left
	alone; if a value appears in 'to' and in 'from', it is
	replaced with 'from'.
 **/
BMessage& arp_update_message(BMessage& to, const BMessage& from);

/**	This function probes the given BView to try to fill in some
	reasonable default values into the given arp_layout_dimens
	structure.  In an ideal world, this would just mean calling
	GetPreferredSize() and copying the result.  Too bad the world
	isn't ideal.
 **/
void get_view_dimens(ArpDimens* dimens, BView* view,
					 bool sizekludge = true);

#endif
