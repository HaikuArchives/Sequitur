/* ArpPoint3dList.h
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
#ifndef ARPMATH_ARPPOINT3DLIST_H
#define ARPMATH_ARPPOINT3DLIST_H

#include <ArpMath/ArpPoint3d.h>

/*******************************************************
 * ARP-POINT-3D-LIST
 * A simple path object for storing a list of 3D coords.
 *
 * WARNING:  The array of points is exposed for fast
 * manipulation, but CLIENTS SHOULD NEVER ALLOCATE OR
 * DEALLOCATE THE ARRAY DIRECTLY.  Use the various
 * memory methods for that.
 *******************************************************/
class ArpPoint3dList
{
public:
	uint32				size;		// Number of points in use
	ArpPoint3d*			pts;

	ArpPoint3dList(uint32 size = 0);
	ArpPoint3dList(const ArpPoint3dList& o);
	~ArpPoint3dList();

	ArpPoint3dList&		operator=(const ArpPoint3dList &o);
	ArpPoint3dList*		Clone() const;
	
	/* MEMORY MANAGEMENT.
	 * Use these methods to alter the size of the pts array,
	  * never do that directly.
	  */
	status_t			ResizeTo(uint32 newSize);
	/* Add to my path, grow if necessary.
	 */
	status_t			Add(float x, float y, float z);
	/* Erase the set of supplied indexes.
	 */
	status_t			Erase(uint32* indexes, uint32 indexesSize);
	
private:
	uint32				mAllocated;		// Number of points allocated

	void				FreeAll();
	
public:
	void				Print() const;
};

#endif
