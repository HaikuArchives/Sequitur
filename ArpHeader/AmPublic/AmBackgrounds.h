/* AmBackgrounds.h
 * Copyright (c)20001 by Eric Hackborn.
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
 * 2001.05.26		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef AMPUBLIC_AMBACKGROUNDS_H
#define AMPUBLIC_AMBACKGROUNDS_H

#include "ArpViews/ArpBackground.h"
#include "AmPublic/AmTimeConverter.h"

/*************************************************************************
 * AM-GRID-BACKGROUND
 * This abstract class draws a grid based on the current grid time
 * (which must be supplied by a subclass).
 *************************************************************************/
class AmGridBackground : public ArpBackground
{
protected:
	virtual void					DrawOn(BView* view, BRect clip);

	virtual AmTime					GridTime() const = 0;
	virtual const AmTimeConverter&	TimeConverter() const = 0;
};

#endif 
