/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpDrawPens.h
 *
 * NOTE: This is probably going to disappear.
 *
 * An ArpDrawPens encapsulates information about standard colors
 * used to draw an application's control areas.  It allows
 * rendering methods to refer to pens as their logical pen
 * function [rather than absolute pen name], so that it is easy
 * to customize their individual appearance and unify color use
 * across a complete application.
 *
 * In addition, the ArpDrawPens object keeps track of four distinct
 * rendering modes -- highlighted, complemented, disabled, and
 * black & white -- which perform mappings between its different
 * colors.  For example, in highlight mode the shadow and shine
 * pens are mapped to each other and background and text pens
 * mapped to the highlight pens, automatically creating a 3D
 * highlight effect.
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
 * 0.1: Created this file.
 *
 */

#pragma once

#ifndef ARPKERNEL_ARPDRAWPENS_H
#define ARPKERNEL_ARPDRAWPENS_H

#ifndef ARPKERNEL_ARPCOLOR_H
#include <ArpKernel/ArpColor.h>
#endif

enum ArpLogicalPen {
	_End_ArpLogicalPen = -1,  // end array of pens
	ArpBackgroundPen = 0,     // basic background color
	ArpTextPen = 1,           // text drawn over background color
	ArpStandoutPen = 2,       // text to stand out on background
	ArpShinePen = 3,          // 3d shine color
	ArpShadowPen = 4,         // 3d shadow color
	ArpBasicFillPen = 5,      // color of basic filled regions
	ArpBasicTextPen = 6,      // text pen in filled regions
	ArpForeFillPen = 7,       // color of foreground filled regions
	ArpForeTextPen = 8,       // text on foreground fills
	ArpHighlightFillPen = 9,  // pen to highlight blocks in
	ArpHighlightTextPen = 10, // text pen over highlighted blocks
	ArpErrorFillPen = 11,     // pen to draw error blocks in
	ArpErrorTextPen = 12,     // text pen over error blocks
	_Num_ArpLogicalPen = 13
};

class ArpDrawPens {
public:
	 /* Really, we should allow a pen count to be passed into the
	  constructor, so that subclasses can define additional
	  application-specific pens. */
	ArpDrawPens() : penMode(0) { InitPens(); }
	ArpDrawPens(ArpDrawPens const& from);
	ArpDrawPens& operator=(ArpDrawPens const& from);

	 // Retrieve a pen color without going through any mode maps
	ArpColor const& GetRawPen(ArpLogicalPen pen) const;
	ArpColor& GetRawPen(ArpLogicalPen pen);

	 // Retrieve a pen color taking into account current mode
	ArpColor const& GetPen(ArpLogicalPen pen) const;
	ArpColor& GetPen(ArpLogicalPen pen);
	ArpColor const& operator[](ArpLogicalPen pen) const { return GetPen(pen); }
	ArpColor& operator[](ArpLogicalPen pen) { return GetPen(pen); }

	 // Set a particular pen color in the object
	void SetPen(ArpLogicalPen pen, ArpColor color) {
	    if( pen < _Num_ArpLogicalPen ) pens[pen] = color;
	}

	 // Set a bunch of pen colors; terminate with _End_ArpLogicalPen.  E.g.:
	 // SetPens(ArpBackgroundPen,ArpColor(0,0,0),
	 //         ArpTextPen,ArpColor(1,1,1),
	 //         _End_ArpLogicalPen);
	void SetPens(ArpLogicalPen pen1, ... );

	 // Initialize pens to default (black and white) values
	void InitPens();

	 // These are the different pen modes we can be in.  Each one has
	 // its own pen mapping; if multiple modes are one, they go through
	 // each consecuative map
	typedef unsigned int PenModeFlag;
	enum {
		PMF_Complement = 1<<0,
		PMF_Highlight = 1<<1,
		PMF_Disable = 1<<2,
		PMF_BlackWhite = 1<<3
	};

	 // Get and set current mode
	void PenMode(PenModeFlag mode) { penMode = mode; }
	PenModeFlag PenMode(void) { return penMode; }

	 // Convenience functions for getting and setting individual modes
	void ComplementMode(int state)
		{ if(state) penMode |= PMF_Complement;
		  else penMode &= ~PMF_Complement; }
	int ComplementMode(void) { return (penMode&PMF_Complement) ? 1 : 0; }
	void HighlightMode(int state)
		{ if(state) penMode |= PMF_Highlight;
		  else penMode &= ~PMF_Highlight; }
	int HighlightMode(void) { return (penMode&PMF_Highlight) ? 1 : 0; }
	void DisableMode(int state)
		{ if(state) penMode |= PMF_Disable;
		  else penMode &= ~PMF_Disable; }
	int DisableMode(void) { return (penMode&PMF_Disable) ? 1 : 0; }
	void BlackWhiteMode(int state)
		{ if(state) penMode |= PMF_BlackWhite;
		  else penMode &= ~PMF_BlackWhite; }
	int BlackWhiteMode(void) { return (penMode&PMF_BlackWhite) ? 1 : 0; }

	 // Get mappings for individual pens (yes, currently these can't
	 // we set; we really should be able to...)
	ArpLogicalPen HighlightPen(ArpLogicalPen pen) {
		return pen >= _Num_ArpLogicalPen ?
			ArpTextPen : (pen <= _End_ArpLogicalPen ?
			             ArpBackgroundPen : highMap[pen]);
	}
	ArpLogicalPen ComplementPen(ArpLogicalPen pen) {
		return pen >= _Num_ArpLogicalPen ?
			ArpTextPen : (pen <= _End_ArpLogicalPen ?
			             ArpBackgroundPen : compMap[pen]);
	}
	ArpLogicalPen DisablePen(ArpLogicalPen pen) {
		return pen >= _Num_ArpLogicalPen ?
			ArpTextPen : (pen <= _End_ArpLogicalPen ?
			             ArpBackgroundPen : disableMap[pen]);
	}
	ArpLogicalPen BlackWhitePen(ArpLogicalPen pen) {
		return pen >= _Num_ArpLogicalPen ?
			ArpTextPen : (pen <= _End_ArpLogicalPen ?
			             ArpBackgroundPen : bwMap[pen]);
	}
	
 protected:
	static ArpLogicalPen highMap[_Num_ArpLogicalPen];
	static ArpLogicalPen compMap[_Num_ArpLogicalPen];
	static ArpLogicalPen disableMap[_Num_ArpLogicalPen];
	static ArpLogicalPen bwMap[_Num_ArpLogicalPen];
	
	ArpColor pens[_Num_ArpLogicalPen];
	PenModeFlag penMode;
};

// The basic drab 'ol gray set of drawing pens.  These are defined as:
//
// ArpBackgroundPen   : Black
// ArpTextPen         : Gray90
// ArpStandoutPen     : Yellow
// ArpShinePen        : White
// ArpShadowPen       : Gray20
// ArpBasicFillPen    : Gray30
// ArpBasicTextPen    : Gray90
// ArpForeFillPen     : Gray70
// ArpForeTextPen     : Black
// ArpHighlightTextPen: Blue
// ArpHighlightTextPen: Black
// ArpErrorFillPen    : LightPink3
// ArpErrorTextPen    : Black

class ArpStdDrawPens : public ArpDrawPens {
public:
	ArpStdDrawPens();
};

// A slightly tinted set of drawing pens, on a black background.
// These are defined as:
//
// ArpBackgroundPen   : Black
// ArpTextPen         : AntiqueWhite
// ArpStandoutPen     : Yellow
// ArpShinePen        : White
// ArpShadowPen       : Gray50
// ArpBasicFillPen    : Bisque4
// ArpBasicTextPen    : AntiqueWhite
// ArpForeFillPen     : AntiqueWhite
// ArpForeTextPen     : Black
// ArpHighlightFillPen: Blue
// ArpHighlightTextPen: Black
// ArpErrorFillPen    : LightPink3
// ArpErrorTextPen    : Black
//
// They're very pretty, don't you think? ;)

class ArpTintDrawPens : public ArpDrawPens {
public:
	ArpTintDrawPens();
};

// A set of drawing pens using the more traditional light background,
// using the same slight tinting.  These are defined as:
//
// ArpBackgroundPen   : AntiqueWhite
// ArpTextPen         : Black
// ArpStandoutPen     : SlateBlue
// ArpShinePen        : White
// ArpShadowPen       : Black
// ArpBasicFillPen    : AntiqueWhite4
// ArpBasicTextPen    : AntiqueWhite
// ArpForeFillPen     : AntiqueWhite
// ArpForeTextPen     : Black
// ArpHighlightTextPen: SlateBlue
// ArpHighlightTextPen: AntiqueWhite
// ArpErrorFillPen    : maroon
// ArpErrorTextPen    : AntiqueWhite

class ArpLightDrawPens : public ArpDrawPens {
public:
	ArpLightDrawPens();
};

#endif
