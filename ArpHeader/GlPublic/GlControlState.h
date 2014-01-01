/* GlControlState.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2004.03.22			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLCONTROLSTATE_H
#define GLPUBLIC_GLCONTROLSTATE_H

#include <GlPublic/GlMidiEvent.h>
#include <GlPublic/GlParamWrap.h>

/***************************************************************************
 * GL-CONTROL-STATE
 * A support class that lets targets communicate about their current state.
 ***************************************************************************/
class GlControlState
{
public:
	/* Targets call this if they can record.
	 */
	virtual void			SetRecordable() = 0;
	/* Targets set which of the MIDI controls they respond to.
	 */
	virtual void			SetMidi(int32 letter) = 0;
};

#endif
