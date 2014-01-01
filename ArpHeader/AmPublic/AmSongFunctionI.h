/* AmSongFunctionI.h
 * Copyright (c)2000 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Eric Hackborn
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
 * 2000/11/26		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMSONGFUNCTIONI_H
#define AMPUBLIC_AMSONGFUNCTIONI_H

#include "AmPublic/AmDefs.h"
class AmSong;

/*************************************************************************
 * AM-SONG-FUNCTION-I
 * This is the definition of an object that may become an add-on.  These
 * objects are provided songs and allowed to process them however they like.
 *************************************************************************/
class AmSongFunctionI
{
public:
	virtual ~AmSongFunctionI();

	virtual const char* Name() const = 0;

	/* Returning true indicates this function object implements the
	 * WriteSong() method.  Returning false indicates that the ReadSong()
	 * method is implemented.
	 */
	virtual bool WriteMode() const = 0;

	virtual void WriteSong(AmSong* song)		{ }
	virtual void ReadSong(const AmSong* song)	{ }
};

#endif 
