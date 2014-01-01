/* GlParamListI.h
 * Copyright (c)2004 by Eric Hackborn.
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
 * 2004.03.22			hackborn@angryredplanet.com
 */
#ifndef GLPUBLIC_GLPARAMLISTI_H
#define GLPUBLIC_GLPARAMLISTI_H

#include <ArpCore/String16.h>
#include <GlPublic/GlDefs.h>
class GlParam;
class GlParamType;

/***************************************************************************
 * GL-PARAM-LIST-I
 * An abstract interface for a parameter-storing object.  These objects
 * are intended to be change targets -- they supply a value-setting
 * interface.
 ****************************************************************************/
class GlParamListI
{
public:
//	uint32			Size() const;
	
	virtual status_t	Add(gl_node_id nid, const GlParam* param, int32 index,
							const BString16* label, int32 control, int32 midi) = 0;
	virtual status_t	Add(gl_node_id nid, const GlParamType* paramType,
							const BString16* label, int32 control, int32 midi) = 0;
};


#endif
