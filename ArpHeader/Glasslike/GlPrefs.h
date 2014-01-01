/* SeqPreferences.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
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
 * 02.24.00		hackborn
 * Created this file.
 */
#ifndef GL_PREFS_H
#define GL_PREFS_H

#include <ArpInterface/ArpPrefs.h>

/***************************************************************************
 * GL-PREFS
 ***************************************************************************/
class GlPrefs : public ArpPrefs
{
public:
	GlPrefs();
	
	virtual void		Initialize();

	virtual int32			GetInt32(uint32 constant) const;
	virtual float			GetFloat(uint32 constant) const;
	virtual BPoint			GetPoint(uint32 constant) const;
	virtual rgb_color		GetColor(uint32 constant) const;
	virtual const ArpFont*	GetFont(uint32 constant) const;

	void 				SetColor(	uint32 constant,
									uint8 red,
									uint8 green,
									uint8 blue,
									uint8 alpha = 255);
	void				SetColor(	uint32 constant,
									rgb_color color);

private:
	int32		mInt32s[ARP_INT32_PREF_SIZE];
	float		mFloats[ARP_FLOAT_PREF_SIZE];
	rgb_color	mColors[ARP_COLOR_PREF_SIZE];

	void		SetColorArray(rgb_color highC, rgb_color lowC, rgb_color* colorArray);
};

#endif
