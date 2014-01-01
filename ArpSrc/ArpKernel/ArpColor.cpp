/*
	
	ArpColor.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef APRKERNEL_ARPCOLOR_H
#include "ArpKernel/ArpColor.h"
#endif

rgb_color mix_color(rgb_color color1, rgb_color color2, float portion)
{
	rgb_color ret;
	ret.red = uint8(color1.red*portion + color2.red*(1-portion) + .5);
	ret.green = uint8(color1.green*portion + color2.green*(1-portion) + .5);
	ret.blue = uint8(color1.blue*portion + color2.blue*(1-portion) + .5);
	ret.alpha = uint8(color1.alpha*portion + color2.alpha*(1-portion) + .5);
	return ret;
}

ArpColor::ArpColor()
{
	color.red = color.green = color.blue = color.alpha = 0;
}

ArpColor::ArpColor(uchar red,uchar green,uchar blue,uchar alpha)
{
	color.red = red;
	color.green = green;
	color.blue = blue;
	color.alpha = alpha;
};

ArpColor::ArpColor(long int mycol)
{
	color.red = (mycol>>16)&0xff;
	color.green = (mycol>>8)&0xff;
	color.blue = mycol&0xff;
	color.alpha = (mycol>>24)&0xff;
}

ArpColor::ArpColor(color_which which)
{
	color = ui_color(which);
}

ArpColor::ArpColor(const rgb_color & mycol)
{
	color = mycol;
};

ArpColor::ArpColor(const ArpColor & mycol)
{
	color = mycol.color;
};

ArpColor::~ArpColor()
{
}

ArpColor::operator rgb_color () {
	return color;
}

ArpColor::operator const rgb_color& () const
{
	return color;
}

ArpColor::operator const rgb_color* () const
{
	return &color;
}

ArpColor& ArpColor::operator = (const ArpColor & set)
{
	color.red = set.color.red;
	color.green = set.color.green;
	color.blue = set.color.blue;
	color.alpha = set.color.alpha;
	return *this;
}
		
ArpColor& ArpColor::operator = (const rgb_color & set)
{
	color.red = set.red;
	color.green = set.green;
	color.blue = set.blue;
	color.alpha = set.alpha;
	return *this;
}
		
bool ArpColor::operator == (const ArpColor & comp) const
{
	return (color.red==comp.color.red
			&& color.green==comp.color.green
			&& color.blue==comp.color.blue
			&& color.alpha==comp.color.alpha);
}
		
bool ArpColor::operator == (const rgb_color & comp) const
{
	return (color.red==comp.red
			&& color.green==comp.green
			&& color.blue==comp.blue
			&& color.alpha==comp.alpha);
}

ArpColor& ArpColor::Tint(float amount)
{
	color = tint_color(color, amount);
	return *this;
}

uchar ArpColor::GetRed(void) { return color.red; }
uchar ArpColor::GetGreen(void) { return color.green; }
uchar ArpColor::GetBlue(void) { return color.blue; }
uchar ArpColor::GetAlpha(void) { return color.alpha; }
float ArpColor::GetGray(void) {
	return ( (color.red+color.green+color.blue)
			 / (3.0*256.0) );
}
	
const ArpColor ArpColor::Black = ArpColor(0x00,0x00,0x00);
const ArpColor ArpColor::White = ArpColor(0xff,0xff,0xff);
const ArpColor ArpColor::Red = ArpColor(0xff,0x00,0x00);
const ArpColor ArpColor::Green = ArpColor(0x00,0xff,0x00);
const ArpColor ArpColor::Blue = ArpColor(0x00,0x00,0xff);
const ArpColor ArpColor::Purple = ArpColor(0xff,0x00,0xff);
const ArpColor ArpColor::Yellow = ArpColor(0xff,0xff,0x00);
const ArpColor ArpColor::Cyan = ArpColor(0x00,0xff,0xff);
const ArpColor ArpColor::Transparent = ArpColor(B_TRANSPARENT_COLOR);
