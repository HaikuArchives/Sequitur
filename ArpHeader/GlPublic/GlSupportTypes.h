/* GlSupportTypes.h
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
 * 2004.04.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLSUPPORTTYPES_H
#define GLPUBLIC_GLSUPPORTTYPES_H

#include <be/app/Message.h>

/***************************************************************************
 * GL-REL-ABS
 * A combination of a float and int32, meant for storing a value in both
 * relative and absolute coords.
 ***************************************************************************/
class GlRelAbs
{
public:
	float		rel;
	int32		abs;

	GlRelAbs(float r = 0.0, int32 a = 0);
	GlRelAbs(const GlRelAbs& o);

	GlRelAbs&		operator=(const GlRelAbs& o);
	bool			operator==(const GlRelAbs& o) const;
	bool			operator!=(const GlRelAbs& o) const;

	void			Set(float r, int32 a);

	status_t		ReadFrom(const BMessage& msg, const char* name);
	status_t		WriteTo(BMessage& msg, const char* name) const;
};

#endif
