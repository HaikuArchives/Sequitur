/* GlMidiTargetI.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.01.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLMIDITARGETI_H
#define GLKERNEL_GLMIDITARGETI_H

#include <GlPublic/GlMidiEvent.h>

/***************************************************************************
 * GL-MIDI-TARGET-I
 * An interface class for anyone receiving MIDI notification.
 ***************************************************************************/
class GlMidiTargetI
{
public:
	virtual ~GlMidiTargetI()			{ }
	
	virtual void			ReceiveMidi(GlMidiEvent event, bigtime_t time) = 0;
	/* Notification that this target's been activated.
	 */
	virtual void			Activate()	{ }
};

#endif
