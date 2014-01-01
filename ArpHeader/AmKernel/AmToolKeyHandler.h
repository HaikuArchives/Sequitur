/* AmToolKeyHandler.h
 * Copyright (c)2002 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.07.28				hackborn@angryredplanet.com
 * Created this file
 */
#ifndef AMKERNEL_AMTOOLKEYHANDLER_H
#define AMKERNEL_AMTOOLKEYHANDLER_H

/***************************************************************************
 * AM-TOOL-KEY-HANDLER
 * An interface that simply acknowledges valid keys.
 ***************************************************************************/
class AmToolKeyHandler
{
public:
	virtual ~AmToolKeyHandler()		{ }

	virtual bool		CanHandle(char byte) const = 0;

protected:
	AmToolKeyHandler()		{ }
};

#endif
