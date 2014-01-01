/* GlPath.h
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
 * 2003.02.11				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLPATH_H
#define GLPUBLIC_GLPATH_H

#include "ArpMath/ArpPoint3d.h"

/*******************************************************
 * GL-PATH
 * A simple path object for storing a list of 3D coords.
 *
 * WARNING:  The array of points is exposed for fast
 * manipulation, but CLIENTS SHOULD NEVER ALLOCATE OR
 * DEALLOCATE THE ARRAY DIRECTLY.  Use the various
 * memory methods for that.
 *******************************************************/
class GlPath
{
public:
	uint32				size;		// Number of points in use
	ArpPoint3d*			pts;

	GlPath(uint32 size = 0);
	GlPath(const GlPath& o);
	~GlPath();

	GlPath&			operator=(const GlPath &o);
	GlPath*			Clone() const;
	
	/* MEMORY MANAGEMENT.
	 * Use these methods to alter the size of the pts array,
	  * never do that directly.
	  */
	status_t		ResizeTo(uint32 newSize);
	/* Add to my path, grow if necessary.
	 */
	status_t		Add(float x, float y, float z);

private:
	uint32			mAllocated;		// Number of points allocated

	void			FreeAll();
	
public:
	void			Print() const;

	/* These are leftovers from when the path data was
	 * hidden.  Left somewhat for backwards compatibility,
	 * somewhat because there's not a lot of reason to
	 * remove them.  If clients are lazy and don't feel
	 * like verifying index, and they aren't performance
	 * critical, then you can use these.  But they probably
	 * should be deprecated.
	 */
	status_t		GetAt(uint32 index, float* outX, float* outY, float* outVal) const;
	status_t		SetAt(uint32 index, float x, float y, float val);
};

#endif
