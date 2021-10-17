/* AmSongFunctionRoster.h
 * Copyright (c)2000 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Eric Hackborn
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Angry Red Planet,
 * at <hackborn@angryredplanet.com> or <hackbod@angryredplanet.com>.
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

#ifndef AMKERNEL_AMSONGFUNCTIONROSTER_H
#define AMKERNEL_AMSONGFUNCTIONROSTER_H

#include "AmPublic/AmSongFunctionI.h"
#include <vector>

/***************************************************************************
 * AM-SONG-FUNCTION-ROSTER
 * This class contains objects that can operate on songs.
 ***************************************************************************/
class AmSongFunctionRoster
{
public:
	AmSongFunctionRoster();
	virtual ~AmSongFunctionRoster();

	static AmSongFunctionRoster* Default();
	static void ShutdownDefault(bool force_unload=false);
	
	AmSongFunctionI* FunctionAt(uint32 index) const;
	AmSongFunctionI* FindFunction(const char* name) const;
	
private:
	std::vector<AmSongFunctionI*>	mFunctions;
};

#endif
