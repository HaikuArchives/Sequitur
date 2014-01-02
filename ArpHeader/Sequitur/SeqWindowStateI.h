/* SeqWindowStateI.h
 * Copyright (c)2000 by Eric Hackborn.
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
 *	* Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	* None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 04.07.00		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQWINDOWSTATEI_H
#define SEQUITUR_SEQWINDOWSTATEI_H

#include <app/Message.h>
#include <interface/Window.h>

/*****************************************************************************
 * SEQ-WINDOW-STATE-I
 * This class defines an interface that any Sequitur windows can implement to
 * hook into certain system features, like persisting the window state between
 * sessions.
 *****************************************************************************/
class SeqWindowStateI
{
public:
	virtual ~SeqWindowStateI() { }

	/* Implementors should answer true if they are a signifcant window --
	 * whenever the last significant window is closed, the app is automatically
	 * closed.
	 */
	virtual bool		IsSignificant() const = 0;
	/* Implementors need to write out their state to the supplied message.
	 */
	virtual status_t	GetConfiguration(BMessage* msg) = 0;

protected:
	/* Convenience functions subclasses can use to get and set the window
	 * size and position.
	 */
	status_t			GetDimensions(BMessage* config, BWindow* window) const;
	status_t			SetDimensions(const BMessage* config, BWindow* window);
};

#endif
