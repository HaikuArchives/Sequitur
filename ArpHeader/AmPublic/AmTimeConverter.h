/* AmTimeConverter.h
 * Copyright (c)1996 - 2000 by Eric Hackborn.
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
 *	• None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.10.00		hackborn
 * Changed the 'PPQN' terminology to 'Tick' which is more accurate.
 *
 * 11.01.98		hackborn
 * Changed PPQNToPixel to send floor() and the new pixel value before returning.
 * That I need to do this is a little odd, I think, I didn't have to before the
 * rewrite.  Will this affect things as the song gets longer?  It was for the
 * MeasureBackground that I did this.
 *
 * 10.27.98		hackborn
 * Added STR_AMTIMECONVERTER define.
 *
 * 10.20.98		hackborn
 * Did some minor code clean-up: Switched the old-style MIDI time datatypes
 * (uint32) to the new AmTime.  Renamed member variables to the new-style
 * m- prefix.
 *
 * 08.10.98		hackborn
 * Mutated this file from its original incarnation.
 */

#ifndef AMKERNEL_AMTIMECONVERTER_H
#define AMKERNEL_AMTIMECONVERTER_H

#include <support/SupportDefs.h>
#include "AmPublic/AmDefs.h"

/***************************************************************************
 * AM-TIME-CONVERTER
 * This is a conversion kit that can be plugged into whomever
 * needs it.  it is for converting MIDI ticks to pixels and vice
 * versa.  The beatConversion var must be set before this class
 * is useful (either via the (long) instantiation method or the
 * SetBeatLength() method).
 * NOTE:  We CAN'T have a beatConversion of 0, that must be avoided.
 * Unrealistic beatConversions tend to produce disastrous results.
 *
 * Realistically, it seems like this should be in view code.
 ***************************************************************************/
// A string to identify my class (when being passed in a BMessage, for example)
#define STR_AMTIMECONVERTER		"AmTimeConverter"

class AmTimeConverter
{
public:
	AmTimeConverter();
	AmTimeConverter(float beatLength);
	virtual ~AmTimeConverter();
		
	// These functions set/get the number of pixels of one beat.
	float BeatLength() const;
	void SetBeatLength(float beatLength);

	/* Given a pixel, answer the MIDI tick that resides at it.
	 */
	AmTime PixelToTick(float pixel) const;
	/* Given a MIDI tick, answer its pixel position in the window.
	 */
	float TickToPixel(AmTime tick) const;

private:
	float		mBeatLength, mBeatConversion;
};

#endif 
