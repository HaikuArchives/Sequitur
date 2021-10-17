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
 * ArpColor.h
 *
 * A class wrapper around rgb_color.
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
 * • Add methods for creating ArpColor objects that are scaled
 *   between two other colors.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 0.1: Created this file.
 *
 */

#ifndef ARPKERNEL_ARPCOLOR_H
#define ARPKERNEL_ARPCOLOR_H

#ifndef _VIEW_H
#include <View.h>
#endif

rgb_color mix_color(rgb_color color1, rgb_color color2, float portion);

class ArpColor {

public:

	ArpColor();
	ArpColor(uchar red,uchar green,uchar blue,uchar alpha=255);
	ArpColor(int32 mycol);
	ArpColor(color_which which);
	ArpColor(const rgb_color & mycol);
	ArpColor(const ArpColor & mycol);
	~ArpColor();

	static const ArpColor Black;
	static const ArpColor White;
	static const ArpColor Red;
	static const ArpColor Green;
	static const ArpColor Blue;
	static const ArpColor Purple;
	static const ArpColor Yellow;
	static const ArpColor Cyan;
	static const ArpColor Transparent;
	
	operator rgb_color ();
	operator const rgb_color& () const;
	operator const rgb_color* () const;
	
	ArpColor& operator = (const ArpColor & set);
	ArpColor& operator = (const rgb_color & set);
	
	bool operator == (const ArpColor & comp) const;
	bool operator == (const rgb_color & comp) const;
	
	ArpColor& Tint(float amount);
	
	uchar GetRed(void);
	uchar GetGreen(void);
	uchar GetBlue(void);
	uchar GetAlpha(void);
	float GetGray(void);
	
private:

	rgb_color color;
	uchar index;
};

#endif
