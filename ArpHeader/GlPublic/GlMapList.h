/* GlMapList.h
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
 * 2004.03.31				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLMAPLIST_H
#define GLPUBLIC_GLMAPLIST_H

#include <GlPublic/GlAlgo1d.h>

/*******************************************************
 * GL-MAP-LIST
 * A simple path object for storing a list of maps.
 *
 * WARNING:  The array of maps is exposed for fast
 * manipulation, but CLIENTS SHOULD NEVER ALLOCATE OR
 * DEALLOCATE THE ARRAY DIRECTLY.  Use the various
 * memory methods for that.
 *******************************************************/
class GlMapList
{
public:
	uint32				size;		// Number of maps in use
	GlAlgo1d**			maps;

	GlMapList(uint32 size = 0);
	GlMapList(const GlMapList& o);
	~GlMapList();

	GlMapList&			operator=(const GlMapList &o);
	GlMapList*			Clone() const;
	
	/* MEMORY MANAGEMENT.
	 * Use these methods to alter the size of the maps array,
	 * never do that directly.
	 */
	enum {
		ABSOLUTE_METHOD	= 0,	// Absolute positions are preserved
								// (indexes are identical regardless of
								// source and destination sizes)
		RELATIVE_METHOD	= 1		// Relative positions are preserved
								// (source and destination sizes are
								// each considered to be of size 1.0)
	};
	status_t			ResizeTo(	uint32 newSize,
									int32 method = ABSOLUTE_METHOD);
	/* Add to my list, grow if necessary.  I take ownership
	 * of the map.
	 */
	status_t			Set(int32 index, GlAlgo1d* map);

private:
	uint32				mAllocated;		// Number of maps allocated

	void				FreeAll();
	
public:
	void				Print() const;
};

#endif
