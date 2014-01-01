/* GlLineCache.h
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
 * 2004.03.19				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLLINECACHE_H
#define GLPUBLIC_GLLINECACHE_H

#include <be/support/SupportDefs.h>

/*******************************************************
 * GL-LINE-CACHE
 * Store a series of offsets, for x and y.  This is a
 * way of representing a line: Each value is an offset
 * from the previous value.  In a real line, the offsets
 * will all have the same sign, and they will never be
 * larger than abs(1).
 *
 * WARNING:  The values are exposed for fast
 * manipulation, but CLIENTS SHOULD NEVER ALLOCATE OR
 * DEALLOCATE THE ARRAYS DIRECTLY.  Use the various
 * memory methods for that.
 *******************************************************/
class GlLineCache
{
public:
	uint32			size;		// Size of the arrays
	int32*			x;
	int32*			y;

	GlLineCache(uint32 size = 0);
	GlLineCache(const GlLineCache& o);
	~GlLineCache();

	GlLineCache&	operator=(const GlLineCache &o);
	GlLineCache*	Clone() const;
	
	/* MEMORY MANAGEMENT.
	 * Use these methods to alter the size of the n array,
	  * never do that directly.
	  */
	status_t		Resize(uint32 newSize);
	/* Add to my set, grow if necessary.
	 */
	status_t		Add(int32 x, int32 y);

private:
	uint32			mAllocated;		// Allocated size of n

	status_t		Copy(const GlLineCache& o);
	void			FreeAll();
	
public:
	void			Print() const;
};

#endif
