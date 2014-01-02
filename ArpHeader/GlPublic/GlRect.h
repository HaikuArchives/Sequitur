/* GlRect.h
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
 * 2003.02.17			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLRECT_H
#define GLPUBLIC_GLRECT_H

#include <support/SupportDefs.h>

/***************************************************************************
 * GL-RECT
 * An int32 rect intended primarily for helping with setting bounds
 * on images.
 ***************************************************************************/
class GlRect
{
public:
	int32		l, t, r, b;

	GlRect();
	GlRect(int32 l, int32 t, int32 r, int32 b);
	GlRect(const GlRect& o);

	bool			operator==(const GlRect& o) const;
	bool			operator!=(const GlRect& o) const;

	/* Make my bounds fit into rect.  If I am invalid (all -1's) then
	 * I am equivalent to rect.  I return an error if the supplied rect
	 * isn't valid for iterating.
	 */
	status_t			ConformTo(const GlRect& rect);
};

#endif
