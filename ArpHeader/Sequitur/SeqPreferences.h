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

#ifndef SEQUITUR_SEQPREFERENCES_H
#define SEQUITUR_SEQPREFERENCES_H

#include "AmPublic/AmPrefsI.h"

/***************************************************************************
 * SEQ-PREFERENCES
 ***************************************************************************/
class SeqPreferences : public AmPreferencesI
{
public:
	SeqPreferences();
	
	virtual void		Initialize();

	virtual int32		Int32(uint32 constant) const;
	virtual float		Size(uint32 constant) const;
	virtual rgb_color	Color(uint32 constant) const;
	virtual rgb_color	PrimaryColor(uint8 velocity = 127) const;
	virtual rgb_color	ShadowColor(uint8 velocity = 127) const;
	virtual rgb_color	SelectedColor(uint8 velocity = 127) const;

	virtual status_t	GetFactoryInt32(const char* fac, const char* view,
										const char* name, int32* outI32, int32 n = 0) const;

	void 				SetColor(	uint32 constant,
									uint8 red,
									uint8 green,
									uint8 blue,
									uint8 alpha = 255);
	void				SetColor(	uint32 constant,
									rgb_color color);

private:
	static const uint32 INT32_LIMIT		= AM_INT32_PREF_END + 5;
	int32		mPrefInt32[INT32_LIMIT];

	static const uint32 SIZE_LIMIT		= AM_SIZE_PREF_END + 5;
	float		mPrefSizes[SIZE_LIMIT];

	static const uint32 COLOR_LIMIT		= AM_COLOR_PREF_END + 5;
	rgb_color	mColors[COLOR_LIMIT];

	enum {
		COLOR_ARRAY_SIZE	= 128
	};
	rgb_color	mPrimary[COLOR_ARRAY_SIZE],
				mSecondary[COLOR_ARRAY_SIZE],
				mSelected[COLOR_ARRAY_SIZE];

	void		SetColorArray(rgb_color highC, rgb_color lowC, rgb_color* colorArray);
};

#endif
