/* AmPianoRollGenerator.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.01.09		hackborn
 * Created this file
 */

#ifndef AMSTDFACTORY_AMPIANOROLLGENERATOR_H
#define AMSTDFACTORY_AMPIANOROLLGENERATOR_H

#include <interface/Bitmap.h>

/*************************************************************************
 * _AM-KEY-DIMENS
 *************************************************************************/
class _AmKeyDimens
{
public:
	_AmKeyDimens() : top(0), bottom(0) { }
	
	float					top, bottom;

	static const uint32		BLACK_KEY_NUM	= 5;
	static const uint32		WHITE_KEY_NUM	= 7;
};

/*************************************************************************
 * AM-PIANO-ROLL-GENERATOR
 *************************************************************************/
class AmPianoRollGenerator
{
public:
	AmPianoRollGenerator(float width);
	virtual ~AmPianoRollGenerator();

	const BBitmap*	PianoRollView(float noteHeight);
	/* Render the supplied key (0 - 11, where 0 == C,
	 * 1 == C#, etc.) onto the supplied view.  BRect r
	 * is the view's bounds.  The current keyboard dimens
	 * should have already been filled via a call to
	 * PianoRollView() with the currect note height.
	 */
	void			RenderKey(BView* view, uint8 key, rgb_color c, BRect r);

private:
	float			mWidth;
	_AmKeyDimens	mBlackKeys[_AmKeyDimens::BLACK_KEY_NUM],
					mWhiteKeys[_AmKeyDimens::WHITE_KEY_NUM];
	
	enum {
		HAS_NO_LOCK		= 0,
		HAS_READ_LOCK	= 1,
		HAS_WRITE_LOCK	= 2
	};
	int32			mLockState;
};

#endif 
